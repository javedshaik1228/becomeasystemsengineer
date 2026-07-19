#include "netforge/packet.hpp"

#include <iostream>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: netforge-packet HEX-BYTES\n";
    return 2;
  }
  const auto bytes = netforge::parse_hex_bytes(argv[1]);
  if (bytes.empty()) {
    std::cerr << "invalid or empty hex input\n";
    return 2;
  }
  const auto summary = netforge::decode_ethernet_ipv4_tcp(bytes);
  if (!summary.valid) {
    std::cerr << "decode failed: " << summary.error << '\n';
    return 1;
  }
  std::cout << summary.source_ip << ':' << summary.source_port << " -> "
            << summary.destination_ip << ':' << summary.destination_port << " flags=0x"
            << std::hex << static_cast<unsigned>(summary.tcp_flags)
            << (summary.vlan_tagged ? " vlan" : "") << '\n';
  return 0;
}
