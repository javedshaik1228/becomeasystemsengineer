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
  bool operator==(const Order&) const = default;
};

struct Fill {
  std::uint64_t taker_id{};
  std::uint64_t maker_id{};
  Side taker_side{Side::buy};
  std::int64_t price_ticks{};
  std::uint64_t quantity{};
  std::uint64_t sequence{};
  bool operator==(const Fill&) const = default;
};

// Step 1 TODO: return false for invalid IDs, symbols, prices, quantities, or sequences.
[[nodiscard]] bool valid_order(const Order& order) noexcept;

}  // namespace matching
