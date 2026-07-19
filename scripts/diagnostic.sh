#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/diagnostic"

CXX_COMMAND=''
if [[ -n "${CXX:-}" ]] && command -v "$CXX" >/dev/null 2>&1; then
  CXX_COMMAND="$CXX"
else
  for candidate in c++ clang++ g++; do
    if command -v "$candidate" >/dev/null 2>&1; then
      CXX_COMMAND="$candidate"
      break
    fi
  done
fi

if [[ -z "$CXX_COMMAND" ]]; then
  printf 'error: no C++ compiler found; complete SETUP.md, then rerun this diagnostic.\n' >&2
  exit 2
fi

mkdir -p "$BUILD_DIR"
printf '[diagnostic] Building with %s\n' "$CXX_COMMAND"
"$CXX_COMMAND" \
  -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  -I "$ROOT_DIR/diagnostic" \
  "$ROOT_DIR/diagnostic/baseline.cpp" \
  "$ROOT_DIR/diagnostic/tests.cpp" \
  -o "$BUILD_DIR/netforge-diagnostic"

printf '[diagnostic] Running six framing and bounds cases\n'
"$BUILD_DIR/netforge-diagnostic"
