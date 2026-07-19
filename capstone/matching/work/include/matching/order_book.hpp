#pragma once

#include "matching/order.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace matching {

class OrderBook {
 public:
  explicit OrderBook(std::string symbol);

  // Steps 2-5 TODOs: resting levels, price-time matching, IOC, and cancellation.
  [[nodiscard]] bool add_resting(Order order);
  [[nodiscard]] std::vector<Fill> submit(Order order);
  [[nodiscard]] bool cancel(std::uint64_t order_id);
  [[nodiscard]] std::optional<std::int64_t> best_bid() const;
  [[nodiscard]] std::optional<std::int64_t> best_ask() const;
  [[nodiscard]] std::size_t resting_orders() const noexcept;

 private:
  std::string symbol_;
  // Choose and justify your own level and cancellation-index representation.
};

}  // namespace matching
