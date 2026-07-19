#include "netforge/packet.hpp"

namespace netforge {

// TODO(day06): bounds-check Ethernet, optional VLAN, IPv4, and TCP headers.
PacketSummary decode_ethernet_ipv4_tcp(std::span<const std::byte>) {
  PacketSummary summary;
  summary.error = "not implemented";
  return summary;
}
std::vector<std::byte> parse_hex_bytes(std::string_view) { return {}; }

}  // namespace netforge
