#pragma once

#include "matching/order_book.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace matching {

class MatchingEngine {
 public:
  explicit MatchingEngine(std::size_t ingress_capacity = 1U << 16U);

  // Call during setup, before the single-consumer hot loop.
  OrderBook& prepare_symbol(const std::string& symbol);
  [[nodiscard]] Ack submit(Order order, std::vector<Fill>& fills);
  [[nodiscard]] bool cancel(std::string_view symbol, std::uint64_t order_id);
  [[nodiscard]] OrderBook& book(const std::string& symbol);

 private:
  std::unordered_map<std::string, std::unique_ptr<OrderBook>> books_;
  std::uint64_t next_sequence_{1};
};

}  // namespace matching
