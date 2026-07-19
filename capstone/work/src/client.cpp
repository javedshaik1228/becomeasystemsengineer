#include "netforge/client.hpp"

namespace netforge {

// TODO(day07): resolve, connect, frame one command, and receive one response.
ClientResult request(std::string_view, std::uint16_t, std::string_view) {
  return {false, {}, "not implemented"};
}

}  // namespace netforge
