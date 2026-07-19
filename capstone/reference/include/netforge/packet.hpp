#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netforge {

struct PacketSummary {
  bool valid{false};
  bool vlan_tagged{false};
  std::string source_ip;
  std::string destination_ip;
  std::uint16_t source_port{0};
  std::uint16_t destination_port{0};
  std::uint8_t tcp_flags{0};
  std::string error;
};

[[nodiscard]] PacketSummary decode_ethernet_ipv4_tcp(std::span<const std::byte> packet);
[[nodiscard]] std::vector<std::byte> parse_hex_bytes(std::string_view text);

}  // namespace netforge
