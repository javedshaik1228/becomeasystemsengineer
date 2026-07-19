// Optional completed reference. Build in capstone/matching/work first.
#include "matching/engine.hpp"
#include "matching/mock_data.hpp"

#include <iostream>

int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "usage: matching-engine orders.csv\n"; return 2; }
  const auto orders = matching::read_mock_orders(argv[1]);
  matching::MatchingEngine engine;
  for (const auto& order : orders) engine.prepare_symbol(order.symbol);
  std::uint64_t fills = 0;
  for (auto order : orders) {
    std::vector<matching::Fill> emitted;
    const auto ack = engine.submit(std::move(order), emitted);
    if (!ack.accepted) { std::cerr << "reject order=" << ack.order_id << " reason=" << ack.reason << '\n'; continue; }
    for (const auto& fill : emitted) {
      ++fills;
      std::cout << fill.sequence << ',' << fill.taker_id << ',' << fill.maker_id << ','
                << (fill.taker_side == matching::Side::buy ? 'B' : 'S') << ','
                << fill.price_ticks << ',' << fill.quantity << '\n';
    }
  }
  std::cerr << "orders=" << orders.size() << " fills=" << fills << '\n';
}
