#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PASS_COUNT=0
WARN_COUNT=0
FAIL_COUNT=0
OPTIONAL_MISSING=()

pass() {
  PASS_COUNT=$((PASS_COUNT + 1))
  printf '[ok]   %s\n' "$*"
}

warn() {
  WARN_COUNT=$((WARN_COUNT + 1))
  printf '[warn] %s\n' "$*"
}

fail() {
  FAIL_COUNT=$((FAIL_COUNT + 1))
  printf '[fail] %s\n' "$*"
}

have() {
  command -v "$1" >/dev/null 2>&1
}

version_at_least() {
  local current="$1"
  local required="$2"
  local current_parts required_parts index current_part required_part
  IFS=. read -r -a current_parts <<<"$current"
  IFS=. read -r -a required_parts <<<"$required"
  for index in 0 1 2; do
    current_part="${current_parts[$index]:-0}"
    required_part="${required_parts[$index]:-0}"
    current_part="${current_part%%[^0-9]*}"
    required_part="${required_part%%[^0-9]*}"
    current_part="${current_part:-0}"
    required_part="${required_part:-0}"
    if ((10#$current_part > 10#$required_part)); then return 0; fi
    if ((10#$current_part < 10#$required_part)); then return 1; fi
  done
  return 0
}

first_line() {
  sed -n '1p' "$1" 2>/dev/null || true
}

printf 'NetForge environment doctor\n'
printf 'Repository: %s\n' "$ROOT_DIR"
printf 'Host: %s %s\n\n' "$(uname -s)" "$(uname -m)"

if [[ "$(uname -s)" == Linux ]] && grep -qi microsoft /proc/version 2>/dev/null; then
  pass 'WSL detected.'
  if [[ "$ROOT_DIR" == /mnt/* ]]; then
    warn 'The repository is on a Windows-mounted drive; repeated C++ builds may be faster in the WSL filesystem.'
  fi
fi

CXX_COMMAND=''
if [[ -n "${CXX:-}" ]] && have "$CXX"; then
  CXX_COMMAND="$CXX"
else
  for candidate in c++ clang++ g++; do
    if have "$candidate"; then
      CXX_COMMAND="$candidate"
      break
    fi
  done
fi

TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/netforge-doctor.XXXXXX")"
trap 'rm -rf -- "$TEMP_DIR"' EXIT

if [[ -z "$CXX_COMMAND" ]]; then
  fail 'No C++ compiler found. Install GCC 12+ or Clang 15+.'
else
  "$CXX_COMMAND" --version >"$TEMP_DIR/compiler-version.txt" 2>&1 || true
  cat >"$TEMP_DIR/cxx20.cpp" <<'CPP'
#include <concepts>
#include <span>
#include <thread>
#include <type_traits>

template <typename T>
concept Number = std::is_arithmetic_v<T>;

int main() {
  int values[] = {1, 2, 3};
  std::span<int> view{values};
  std::jthread worker([] {});
  static_assert(Number<int>);
  return view.size() == 3 ? 0 : 1;
}
CPP
  if "$CXX_COMMAND" -std=c++20 -pthread "$TEMP_DIR/cxx20.cpp" -o "$TEMP_DIR/cxx20" >"$TEMP_DIR/cxx20.log" 2>&1; then
    pass "C++20 compiler works: $(first_line "$TEMP_DIR/compiler-version.txt")"
  else
    fail "Compiler '$CXX_COMMAND' cannot build the C++20 probe."
    sed -n '1,8p' "$TEMP_DIR/cxx20.log"
  fi

  cat >"$TEMP_DIR/sanitizer.cpp" <<'CPP'
#include <mutex>
#include <vector>

int main() {
  std::mutex mutex;
  std::vector<int> values;
  {
    std::scoped_lock lock{mutex};
    values.push_back(42);
  }
  return values.front() == 42 ? 0 : 1;
}
CPP
  if "$CXX_COMMAND" -std=c++20 -pthread -O1 -g -fno-omit-frame-pointer \
      -fsanitize=address,undefined "$TEMP_DIR/sanitizer.cpp" -o "$TEMP_DIR/asan-ubsan" \
      >"$TEMP_DIR/asan-ubsan-build.log" 2>&1; then
    if "$TEMP_DIR/asan-ubsan" >"$TEMP_DIR/asan-ubsan-run.log" 2>&1; then
      pass 'AddressSanitizer and UndefinedBehaviorSanitizer compile and run.'
    else
      warn 'ASan/UBSan compile, but the runtime probe failed in this host environment.'
      sed -n '1,5p' "$TEMP_DIR/asan-ubsan-run.log"
    fi
  else
    fail 'AddressSanitizer/UndefinedBehaviorSanitizer are unavailable with the selected compiler.'
    sed -n '1,5p' "$TEMP_DIR/asan-ubsan-build.log"
  fi

  if "$CXX_COMMAND" -std=c++20 -pthread -O1 -g -fno-omit-frame-pointer \
      -fsanitize=thread "$TEMP_DIR/sanitizer.cpp" -o "$TEMP_DIR/tsan" \
      >"$TEMP_DIR/tsan-build.log" 2>&1; then
    if "$TEMP_DIR/tsan" >"$TEMP_DIR/tsan-run.log" 2>&1; then
      pass 'ThreadSanitizer compiles and runs.'
    else
      warn 'ThreadSanitizer compiles, but its runtime is unavailable in this host environment.'
      sed -n '1,5p' "$TEMP_DIR/tsan-run.log"
    fi
  else
    warn 'ThreadSanitizer is unavailable with the selected compiler.'
    sed -n '1,5p' "$TEMP_DIR/tsan-build.log"
  fi
fi

# Day 12's Core gate uses LLVM libFuzzer, so a generic C++20 compiler is not
# sufficient for the complete course contract.
if ! have clang++; then
  fail 'clang++ is missing; Day 12 requires LLVM libFuzzer with Clang.'
else
  cat >"$TEMP_DIR/fuzzer.cpp" <<'CPP'
#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t*, std::size_t) {
  return 0;
}
CPP
  if clang++ -std=c++20 -fsanitize=fuzzer,address,undefined \
      "$TEMP_DIR/fuzzer.cpp" -o "$TEMP_DIR/fuzzer-probe" \
      >"$TEMP_DIR/fuzzer-build.log" 2>&1; then
    pass 'Clang can build the Day 12 libFuzzer + sanitizer probe.'
  else
    fail 'Clang is present, but the Day 12 libFuzzer + sanitizer probe cannot build.'
    sed -n '1,8p' "$TEMP_DIR/fuzzer-build.log"
  fi
fi

if have python3; then
  pass "Python is available: $(python3 --version 2>&1)."
else
  fail 'python3 is missing; the practical diagnostic and role-screen runner require it.'
fi

if have clang-tidy; then
  pass "clang-tidy is available: $(clang-tidy --version | sed -n '1p')."
else
  fail 'clang-tidy is missing; Day 10 and the Day 14 static-analysis gate require it.'
fi

if have cmake; then
  CMAKE_VERSION="$(cmake --version | awk 'NR == 1 { print $3 }')"
  if version_at_least "$CMAKE_VERSION" '3.24.0'; then
    pass "CMake $CMAKE_VERSION (minimum 3.24)."
  else
    fail "CMake $CMAKE_VERSION is too old; NetForge requires 3.24 or newer."
  fi
else
  fail 'CMake is missing.'
fi

if have ctest; then
  pass "CTest is available: $(ctest --version | awk 'NR == 1 { print $3 }')."
else
  fail 'CTest is missing; it normally ships with CMake.'
fi

if have ninja; then
  pass "Ninja is available: $(ninja --version)."
elif have make; then
  pass 'Make is available; Ninja is optional but recommended for faster incremental builds.'
else
  fail 'No supported build tool found; install Ninja or Make.'
fi

DEBUGGERS=()
if have gdb; then DEBUGGERS+=("gdb $(gdb --version | sed -n '1s/^GNU gdb //p')"); fi
if have lldb; then DEBUGGERS+=("lldb $(lldb --version | sed -n '1s/.*version //p')"); fi
if ((${#DEBUGGERS[@]} > 0)); then
  pass "Debugger available: ${DEBUGGERS[*]}."
else
  fail 'No debugger found; install GDB on Linux/WSL or LLDB on macOS.'
fi

if have tcpdump; then
  pass 'tcpdump is available (capturing may still require sudo or Linux capabilities).'
else
  fail 'tcpdump is missing.'
fi

for optional in git node clangd clang-format rg; do
  if ! have "$optional"; then OPTIONAL_MISSING+=("$optional"); fi
done

case "$(uname -s)" in
  Linux)
    for optional in valgrind strace ltrace perf tshark; do
      if ! have "$optional"; then OPTIONAL_MISSING+=("$optional"); fi
    done
    ;;
  Darwin)
    for optional in dtruss leaks tshark; do
      if ! have "$optional"; then OPTIONAL_MISSING+=("$optional"); fi
    done
    if ! xcrun -f xctrace >/dev/null 2>&1; then OPTIONAL_MISSING+=("xctrace"); fi
    ;;
esac

if ((${#OPTIONAL_MISSING[@]} == 0)); then
  pass 'All optional diagnostics and course-support tools are available.'
else
  warn "Optional tools not found: ${OPTIONAL_MISSING[*]}. Core work can continue unless a lesson names one."
fi

printf '\nSummary: %d passed, %d warning(s), %d failure(s).\n' \
  "$PASS_COUNT" "$WARN_COUNT" "$FAIL_COUNT"

if ((FAIL_COUNT > 0)); then
  exit 1
fi
