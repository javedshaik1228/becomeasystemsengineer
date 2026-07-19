#!/usr/bin/env bash
set -euo pipefail

track="${1:-work}"
requests="${2:-1000}"
concurrency="${3:-8}"
warmup="${NETFORGE_BENCH_WARMUP:-10}"
port="${NETFORGE_PORT:-39090}"

if [[ "$track" != "work" && "$track" != "reference" ]]; then
  echo "usage: $0 work|reference [requests] [concurrency]" >&2
  exit 2
fi
if ! [[ "$requests" =~ ^[1-9][0-9]*$ && "$concurrency" =~ ^[1-9][0-9]*$ &&
        "$warmup" =~ ^[0-9]+$ ]]; then
  echo "requests/concurrency must be positive integers and warmup must be non-negative" >&2
  exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build/$track"
server="$build_dir/netforge-server"
client="$build_dir/netforge-client"
bench="$build_dir/netforge-bench"

if [[ ! -x "$server" || ! -x "$client" || ! -x "$bench" ]]; then
  echo "Build the $track track through Day 08 first: ./scripts/check.sh $track day08" >&2
  exit 1
fi

log_file="$(mktemp -t netforge-bench.XXXXXX)"
server_pid=""
cleanup() {
  if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
    kill -INT "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
  rm -f "$log_file"
}
trap cleanup EXIT INT TERM

"$server" "$port" >"$log_file" 2>&1 &
server_pid=$!

ready=0
for _ in $(seq 1 50); do
  if "$client" 127.0.0.1 "$port" PING >/dev/null 2>&1; then
    ready=1
    break
  fi
  if ! kill -0 "$server_pid" 2>/dev/null; then break; fi
  sleep 0.1
done
if [[ "$ready" -ne 1 ]]; then
  echo "server did not become ready on port $port" >&2
  sed -n '1,20p' "$log_file" >&2
  exit 1
fi

"$bench" 127.0.0.1 "$port" "$requests" "$concurrency" "$warmup"
