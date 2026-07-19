// Optional completed reference. Build in capstone/matching/work first.
#pragma once

#include <cstdint>
#include <string>

namespace matching {

enum class Side : std::uint8_t { buy, sell };
enum class TimeInForce : std::uint8_t { good_till_cancel, immediate_or_cancel };

struct Order {
  std::uint64_t id{};
  std::string symbol;
  Side side{Side::buy};
  std::int64_t price_ticks{};
  std::uint64_t quantity{};
  std::uint64_t sequence{};
  TimeInForce tif{TimeInForce::good_till_cancel};
};

struct Fill {
  std::uint64_t taker_id{};
  std::uint64_t maker_id{};
  Side taker_side{Side::buy};
  std::int64_t price_ticks{};
  std::uint64_t quantity{};
  std::uint64_t sequence{};
};

struct Ack {
  std::uint64_t order_id{};
  bool accepted{false};
  std::uint64_t remaining{};
  std::string reason;
};

}  // namespace matching
