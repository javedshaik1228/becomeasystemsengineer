// Optional completed reference. Build in capstone/matching/work first.
#include "matching/engine.hpp"

#include <utility>

namespace matching {

MatchingEngine::MatchingEngine(std::size_t) {}

OrderBook& MatchingEngine::prepare_symbol(const std::string& symbol) {
  return book(symbol);
}

OrderBook& MatchingEngine::book(const std::string& symbol) {
  auto found = books_.find(symbol);
  if (found == books_.end()) found = books_.emplace(symbol, std::make_unique<OrderBook>(symbol)).first;
  return *found->second;
}

Ack MatchingEngine::submit(Order order, std::vector<Fill>& fills) {
  if (order.sequence == 0) order.sequence = next_sequence_++;
  return book(order.symbol).submit(std::move(order), fills);
}

bool MatchingEngine::cancel(std::string_view symbol, std::uint64_t order_id) {
  return book(std::string(symbol)).cancel(order_id);
}

}  // namespace matching
