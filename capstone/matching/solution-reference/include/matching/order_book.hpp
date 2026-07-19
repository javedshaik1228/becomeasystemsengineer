// Optional completed reference. Build in capstone/matching/work first.
#pragma once

#include "matching/order.hpp"

#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace matching {

class OrderBook {
 public:
  explicit OrderBook(std::string symbol);

  [[nodiscard]] const std::string& symbol() const noexcept { return symbol_; }
  [[nodiscard]] Ack submit(Order order, std::vector<Fill>& fills);
  [[nodiscard]] bool cancel(std::uint64_t order_id);
  [[nodiscard]] std::optional<std::int64_t> best_bid() const;
  [[nodiscard]] std::optional<std::int64_t> best_ask() const;
  [[nodiscard]] std::uint64_t resting_orders() const noexcept { return live_orders_.size(); }

 private:
  struct Level {
    std::deque<Order> orders;
  };

  using Bids = std::map<std::int64_t, Level, std::greater<>>;
  using Asks = std::map<std::int64_t, Level>;

  void erase_empty_levels();
  std::string symbol_;
  Bids bids_;
  Asks asks_;
  std::unordered_map<std::uint64_t, Side> live_orders_;
};

}  // namespace matching
