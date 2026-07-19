#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TRACK="${1:-work}"
SELECTOR="${2:-all}"

usage() {
  cat <<'USAGE'
Usage: scripts/check.sh [work|reference] [all|DAY|REGEX]

Examples:
  scripts/check.sh work all
  scripts/check.sh work 07
  scripts/check.sh reference day-11
  scripts/check.sh work 'day[-_]?0[4-6]'

The second argument is passed to CTest as a test-name regex unless it is a day
number or day-NN shorthand. The build trees are isolated by track.
USAGE
}

if [[ "$TRACK" == '-h' || "$TRACK" == '--help' ]]; then
  usage
  exit 0
fi

if [[ "$TRACK" != work && "$TRACK" != reference ]]; then
  printf 'error: track must be work or reference, got %q\n' "$TRACK" >&2
  usage >&2
  exit 2
fi

SOURCE_DIR="$ROOT_DIR/capstone"
BUILD_DIR="$ROOT_DIR/build/$TRACK"

if [[ ! -f "$SOURCE_DIR/CMakeLists.txt" ]]; then
  printf 'error: expected %s\n' "$SOURCE_DIR/CMakeLists.txt" >&2
  printf 'The capstone source tree has not been deployed yet.\n' >&2
  exit 2
fi

CONFIGURE_ARGS=(
  -S "$SOURCE_DIR"
  -B "$BUILD_DIR"
  -DNETFORGE_TRACK="$TRACK"
  -DCMAKE_BUILD_TYPE=Debug
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]] && command -v ninja >/dev/null 2>&1; then
  CONFIGURE_ARGS+=(-G Ninja)
fi

printf '[check] Configuring %s track in %s\n' "$TRACK" "$BUILD_DIR"
cmake "${CONFIGURE_ARGS[@]}"

printf '[check] Building %s track\n' "$TRACK"
cmake --build "$BUILD_DIR" --parallel

if [[ "$SELECTOR" == all ]]; then
  printf '[check] Running all tests for %s\n' "$TRACK"
  ctest --test-dir "$BUILD_DIR" --output-on-failure --no-tests=error
  exit $?
fi

TEST_REGEX="$SELECTOR"
if [[ "$SELECTOR" =~ ^([Dd][Aa][Yy][:_-]?)?([0-9]{1,2})$ ]]; then
  DAY_NUMBER=$((10#${BASH_REMATCH[2]}))
  if ((DAY_NUMBER < 1 || DAY_NUMBER > 14)); then
    printf 'error: day must be between 1 and 14, got %s\n' "$DAY_NUMBER" >&2
    exit 2
  fi
  printf -v DAY_PADDED '%02d' "$DAY_NUMBER"
  TEST_REGEX="[Dd][Aa][Yy][-_]?(${DAY_PADDED}|${DAY_NUMBER})([^0-9]|$)"
fi

printf '[check] Running tests matching regex: %s\n' "$TEST_REGEX"
ctest --test-dir "$BUILD_DIR" --output-on-failure --no-tests=error -R "$TEST_REGEX"
