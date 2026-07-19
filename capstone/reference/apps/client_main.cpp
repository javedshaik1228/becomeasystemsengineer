#include "netforge/client.hpp"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string_view>
#include <system_error>

namespace {

bool parse_port(std::string_view text, std::uint16_t& port) {
  std::uint32_t value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size() || value == 0 ||
      value > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  port = static_cast<std::uint16_t>(value);
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: netforge-client HOST PORT COMMAND [ARG ...]\n";
    return 2;
  }
  std::uint16_t port = 0;
  if (!parse_port(argv[2], port)) {
    std::cerr << "invalid port: " << argv[2] << '\n';
    return 2;
  }
  std::ostringstream command;
  for (int index = 3; index < argc; ++index) {
    if (index != 3) command << ' ';
    command << argv[index];
  }
  const auto result = netforge::request(argv[1], port, command.str());
  if (!result.ok) {
    std::cerr << "request failed: " << result.error << '\n';
    return 1;
  }
  std::cout << result.response << '\n';
  return 0;
}
