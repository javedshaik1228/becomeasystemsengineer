#!/usr/bin/env bash
set -euo pipefail
track="${1:-work}"
step="${2:-1}"

case "$track" in
  work)
    build_dir="build/matching-work"
    cmake -S capstone/matching/work -B "$build_dir" -DMATCHING_STEP="$step" -DCMAKE_BUILD_TYPE=Debug
    ;;
  solution-reference)
    build_dir="build/matching-reference"
    cmake -S capstone/matching/solution-reference -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug -DMATCHING_WITH_RDKAFKA=OFF
    ;;
  *)
    echo "usage: scripts/check-matching.sh [work STEP|solution-reference]" >&2
    exit 2
    ;;
esac

cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure
