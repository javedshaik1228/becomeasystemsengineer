#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TRACK="${1:-work}"

if [[ "$TRACK" != work && "$TRACK" != reference ]]; then
  printf 'usage: %s [work|reference]\n' "$0" >&2
  exit 2
fi

CLANG_TIDY=''
if command -v clang-tidy >/dev/null 2>&1; then
  CLANG_TIDY="$(command -v clang-tidy)"
elif [[ "$(uname -s)" == Darwin ]] && command -v brew >/dev/null 2>&1; then
  LLVM_PREFIX="$(brew --prefix llvm 2>/dev/null || true)"
  if [[ -x "$LLVM_PREFIX/bin/clang-tidy" ]]; then
    CLANG_TIDY="$LLVM_PREFIX/bin/clang-tidy"
  fi
fi

if [[ -z "$CLANG_TIDY" ]]; then
  printf 'error: clang-tidy is unavailable; run scripts/bootstrap.sh or install it manually.\n' >&2
  exit 1
fi

BUILD_DIR="$ROOT_DIR/build/$TRACK-analysis"
SOURCE_DIR="$ROOT_DIR/capstone"
printf '[analyze] Configuring %s compile database\n' "$TRACK"
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
  -DNETFORGE_TRACK="$TRACK" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

SOURCES=()
while IFS= read -r source; do
  SOURCES+=("$source")
done < <(find "$SOURCE_DIR/$TRACK" "$SOURCE_DIR/tests" -type f -name '*.cpp' -print | sort)

if ((${#SOURCES[@]} == 0)); then
  printf 'error: no C++ sources found for %s\n' "$TRACK" >&2
  exit 1
fi

printf '[analyze] Running clang-tidy on %d translation unit(s)\n' "${#SOURCES[@]}"
"$CLANG_TIDY" \
  -p "$BUILD_DIR" \
  --checks='-*,clang-analyzer-*,bugprone-*,performance-*,portability-*' \
  --warnings-as-errors='clang-analyzer-*' \
  --header-filter=".*/capstone/$TRACK/(include|src|apps)/.*" \
  "${SOURCES[@]}"

printf '[analyze] PASS: no clang-analyzer finding remains. Review non-error diagnostics before interview use.\n'
