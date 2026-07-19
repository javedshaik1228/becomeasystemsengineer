#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct DecodeResult {
  bool ok{false};
  std::string_view payload;
  std::string error;
  std::size_t allocation_bytes{0};
};

std::uint32_t decode_length(std::span<const std::byte> bytes) {
  return (std::to_integer<std::uint32_t>(bytes[0]) << 24U) |
         (std::to_integer<std::uint32_t>(bytes[1]) << 16U) |
         (std::to_integer<std::uint32_t>(bytes[2]) << 8U) |
         std::to_integer<std::uint32_t>(bytes[3]);
}

DecodeResult decode_frame(std::span<const std::byte> bytes, std::size_t max_payload) {
  if (bytes.size() < 4) return {false, {}, "truncated header", 0};

  const auto declared = static_cast<std::size_t>(decode_length(bytes.first(4)));
  std::string scratch(declared, '\0');
  const auto allocation_bytes = scratch.capacity();

  if (declared > max_payload) return {false, {}, "frame too large", allocation_bytes};
  if (bytes.size() - 4 < declared) return {false, {}, "truncated body", allocation_bytes};

  std::transform(bytes.begin() + 4, bytes.begin() + 4 + declared, scratch.begin(),
                 [](std::byte value) { return static_cast<char>(std::to_integer<unsigned>(value)); });
  return {true, std::string_view(scratch), {}, allocation_bytes};
}

std::vector<std::byte> make_frame(std::string_view payload) {
  const auto length = static_cast<std::uint32_t>(payload.size());
  std::vector<std::byte> bytes(4 + payload.size());
  bytes[0] = static_cast<std::byte>((length >> 24U) & 0xffU);
  bytes[1] = static_cast<std::byte>((length >> 16U) & 0xffU);
  bytes[2] = static_cast<std::byte>((length >> 8U) & 0xffU);
  bytes[3] = static_cast<std::byte>(length & 0xffU);
  std::transform(payload.begin(), payload.end(), bytes.begin() + 4,
                 [](unsigned char value) { return static_cast<std::byte>(value); });
  return bytes;
}

int oversize_case() {
  const auto bytes = make_frame(std::string(64, 'x'));
  const auto result = decode_frame(bytes, 8);
  if (result.ok || result.error != "frame too large") {
    std::cerr << "FAIL: oversized frame did not return the bounded error\n";
    return 1;
  }
  if (result.allocation_bytes != 0) {
    std::cerr << "FAIL: oversized length caused " << result.allocation_bytes
              << " bytes of payload work before rejection\n";
    return 1;
  }
  std::cout << "oversize: PASS\n";
  return 0;
}

int lifetime_case() {
  const std::string expected(256, 'q');
  const auto result = decode_frame(make_frame(expected), 512);
  if (!result.ok) {
    std::cerr << "FAIL: valid frame was rejected: " << result.error << '\n';
    return 1;
  }
  std::vector<std::string> allocator_churn(64, std::string(256, 'z'));
  const std::string observed(result.payload);
  if (observed != expected) {
    std::cerr << "FAIL: returned payload no longer names valid owned bytes\n";
    return 1;
  }
  std::cout << "lifetime: PASS (churn blocks=" << allocator_churn.size() << ")\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mock-a oversize|lifetime\n";
    return 2;
  }
  const std::string_view mode(argv[1]);
  if (mode == "oversize") return oversize_case();
  if (mode == "lifetime") return lifetime_case();
  std::cerr << "unknown mode: " << mode << '\n';
  return 2;
}

