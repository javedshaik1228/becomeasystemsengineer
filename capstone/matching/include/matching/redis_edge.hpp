#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace matching {

struct EdgeResult {
  bool ok{false};
  std::string error;
};

// Minimal RESP2 publisher. It is intentionally synchronous and off the hot
// matching path: production wiring should put this behind an egress queue.
class RedisPublisher {
 public:
  RedisPublisher(std::string host, std::uint16_t port, std::string channel);
  [[nodiscard]] EdgeResult publish(std::string_view payload) const;

 private:
  std::string host_;
  std::uint16_t port_;
  std::string channel_;
};

}  // namespace matching
