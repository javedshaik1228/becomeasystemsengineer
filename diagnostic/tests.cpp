#include "baseline.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using netforge::diagnostic::HeaderStatus;

int failures = 0;

void expect(bool condition, std::string_view message) {
  if (condition) return;
  ++failures;
  std::cerr << "FAIL: " << message << '\n';
}

std::vector<std::byte> bytes(std::initializer_list<unsigned> values) {
  std::vector<std::byte> result;
  result.reserve(values.size());
  for (const auto value : values) result.push_back(static_cast<std::byte>(value));
  return result;
}

}  // namespace

int main() {
  using netforge::diagnostic::inspect_first_frame;

  const std::array<std::byte, 3> short_header{};
  expect(inspect_first_frame(short_header, 1024).status == HeaderStatus::incomplete,
         "fewer than four header bytes are incomplete");

  const auto split_payload = bytes({0, 0, 0, 5, 'h', 'i'});
  expect(inspect_first_frame(split_payload, 1024).status == HeaderStatus::incomplete,
         "a complete header does not imply a complete payload");

  const auto complete = bytes({0, 0, 0, 5, 'h', 'e', 'l', 'l', 'o'});
  const auto complete_result = inspect_first_frame(complete, 1024);
  expect(complete_result.status == HeaderStatus::ready && complete_result.frame_bytes == 9,
         "big-endian length yields the exact first-frame byte count");

  const auto empty = bytes({0, 0, 0, 0});
  const auto empty_result = inspect_first_frame(empty, 1024);
  expect(empty_result.status == HeaderStatus::ready && empty_result.frame_bytes == 4,
         "a zero-length payload is a complete four-byte frame");

  const auto oversized = bytes({0, 0, 4, 1});
  expect(inspect_first_frame(oversized, 1024).status == HeaderStatus::too_large,
         "an oversized length is rejected before payload allocation");

  const auto two_frames = bytes({0, 0, 0, 1, 'a', 0, 0, 0, 1, 'b'});
  const auto first = inspect_first_frame(two_frames, 1024);
  expect(first.status == HeaderStatus::ready && first.frame_bytes == 5,
         "inspection stops exactly after the first complete frame");

  if (failures == 0) {
    std::cout << "Practical diagnostic: PASS\n";
    return 0;
  }
  std::cerr << failures << " diagnostic case(s) remain\n";
  return 1;
}
