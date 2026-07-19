#pragma once

#include "matching/order_book.hpp"

#include <string>
#include <unordered_map>

namespace matching {

class MatchingEngine {
 public:
  void prepare_symbol(const std::string& symbol);
  [[nodiscard]] std::vector<Fill> submit(Order order);

 private:
  // Step 6 TODO: preserve a single-writer sequencing contract across books.
  std::unordered_map<std::string, OrderBook> books_;
  std::uint64_t next_sequence_{1};
};

}  // namespace matching
