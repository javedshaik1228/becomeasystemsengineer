#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace netforge {

struct ClientResult {
  bool ok{false};
  std::string response;
  std::string error;
};

[[nodiscard]] ClientResult request(std::string_view host, std::uint16_t port,
                                   std::string_view command);

}  // namespace netforge
