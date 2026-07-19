#include "netforge/packet.hpp"

#include <arpa/inet.h>

#include <array>
#include <charconv>
#include <cctype>
#include <cstring>
#include <sstream>

namespace netforge {
namespace {

std::uint16_t read_u16(std::span<const std::byte> bytes, std::size_t offset) {
  const auto high = static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset]));
  const auto low = static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset + 1]));
  return static_cast<std::uint16_t>((high << 8U) | low);
}

std::string format_ipv4(std::span<const std::byte> bytes, std::size_t offset) {
  std::ostringstream output;
  for (std::size_t index = 0; index < 4; ++index) {
    if (index != 0) output << '.';
    output << static_cast<unsigned>(std::to_integer<unsigned char>(bytes[offset + index]));
  }
  return output.str();
}

}  // namespace

PacketSummary decode_ethernet_ipv4_tcp(std::span<const std::byte> packet) {
  PacketSummary summary;
  if (packet.size() < 14) {
    summary.error = "truncated Ethernet header";
    return summary;
  }

  std::size_t ip_offset = 14;
  auto ether_type = read_u16(packet, 12);
  if (ether_type == 0x8100 || ether_type == 0x88A8) {
    if (packet.size() < 18) {
      summary.error = "truncated VLAN header";
      return summary;
    }
    summary.vlan_tagged = true;
    ether_type = read_u16(packet, 16);
    ip_offset = 18;
  }
  if (ether_type != 0x0800) {
    summary.error = "EtherType is not IPv4";
    return summary;
  }
  if (packet.size() < ip_offset + 20) {
    summary.error = "truncated IPv4 header";
    return summary;
  }

  const auto version_ihl = std::to_integer<unsigned char>(packet[ip_offset]);
  if ((version_ihl >> 4U) != 4U) {
    summary.error = "IP version is not 4";
    return summary;
  }
  const auto ip_header_length = static_cast<std::size_t>(version_ihl & 0x0FU) * 4U;
  if (ip_header_length < 20 || packet.size() < ip_offset + ip_header_length) {
    summary.error = "invalid IPv4 header length";
    return summary;
  }
  const auto total_length = static_cast<std::size_t>(read_u16(packet, ip_offset + 2));
  if (total_length < ip_header_length + 20 || packet.size() < ip_offset + total_length) {
    summary.error = "invalid or truncated IPv4 total length";
    return summary;
  }
  if (std::to_integer<unsigned char>(packet[ip_offset + 9]) != 6U) {
    summary.error = "IPv4 payload is not TCP";
    return summary;
  }
  const auto flags_and_fragment_offset = read_u16(packet, ip_offset + 6);
  if ((flags_and_fragment_offset & 0x3FFFU) != 0U) {
    summary.error = "fragmented IPv4 packets do not contain a complete TCP header";
    return summary;
  }

  const auto tcp_offset = ip_offset + ip_header_length;
  const auto tcp_header_length =
      static_cast<std::size_t>(std::to_integer<unsigned char>(packet[tcp_offset + 12]) >> 4U) * 4U;
  if (tcp_header_length < 20 || tcp_offset + tcp_header_length > ip_offset + total_length) {
    summary.error = "invalid TCP header length";
    return summary;
  }

  summary.source_ip = format_ipv4(packet, ip_offset + 12);
  summary.destination_ip = format_ipv4(packet, ip_offset + 16);
  summary.source_port = read_u16(packet, tcp_offset);
  summary.destination_port = read_u16(packet, tcp_offset + 2);
  summary.tcp_flags = std::to_integer<std::uint8_t>(packet[tcp_offset + 13]);
  summary.valid = true;
  return summary;
}

std::vector<std::byte> parse_hex_bytes(std::string_view text) {
  std::vector<std::byte> bytes;
  int high_nibble = -1;
  for (const char character : text) {
    if (std::isspace(static_cast<unsigned char>(character)) != 0 || character == ':' ||
        character == '-') {
      continue;
    }
    int value = -1;
    if (character >= '0' && character <= '9') value = character - '0';
    if (character >= 'a' && character <= 'f') value = character - 'a' + 10;
    if (character >= 'A' && character <= 'F') value = character - 'A' + 10;
    if (value < 0) return {};
    if (high_nibble < 0) {
      high_nibble = value;
    } else {
      bytes.push_back(static_cast<std::byte>((high_nibble << 4) | value));
      high_nibble = -1;
    }
  }
  if (high_nibble >= 0) return {};
  return bytes;
}

}  // namespace netforge
