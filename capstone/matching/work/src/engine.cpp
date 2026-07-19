#include "matching/engine.hpp"

namespace matching {

void MatchingEngine::prepare_symbol(const std::string& symbol) {
  books_.try_emplace(symbol, symbol);
}

std::vector<Fill> MatchingEngine::submit(Order) { return {}; }

}  // namespace matching
