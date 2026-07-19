#!/usr/bin/env bash
set -euo pipefail
build_dir="${1:-build/matching}"
cmake -S capstone/matching -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure
