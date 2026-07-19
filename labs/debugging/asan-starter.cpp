#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

std::span<const std::uint8_t> decoded_payload() {
  std::vector<std::uint8_t> frame(4096, 7);
  frame.front() = 42;
  return {frame.data(), frame.size()};  // Intentionally dangles.
}

int main() {
  const auto payload = decoded_payload();
  std::uint64_t checksum = 0;
  for (const auto byte : payload) checksum += byte;
  std::cout << "bytes=" << payload.size() << " checksum=" << checksum << '\n';
  return checksum == 28'707 ? 0 : 1;
}

