#include "matching/engine.hpp"
#include "matching/edge.hpp"
#include "matching/ingress_queue.hpp"
#include "matching/mock_data.hpp"

#include <cassert>
#include <iostream>

int main() {
  using matching::Order;
  using matching::Side;

  assert(matching::valid_order({1, "ACME", Side::buy, 100, 5, 1}));
  assert(!matching::valid_order({0, "ACME", Side::buy, 100, 5, 1}));
  assert(!matching::valid_order({2, "", Side::buy, 100, 5, 2}));
  assert(!matching::valid_order({3, "ACME", Side::buy, 0, 5, 3}));

#if MATCHING_STEP >= 2
  matching::OrderBook levels("ACME");
  assert(levels.add_resting({10, "ACME", Side::buy, 100, 4, 10}));
  assert(levels.add_resting({11, "ACME", Side::sell, 102, 2, 11}));
  assert(levels.best_bid() == 100 && levels.best_ask() == 102);
#endif

#if MATCHING_STEP >= 3
  assert(levels.add_resting({12, "ACME", Side::sell, 102, 3, 12}));
  const auto fills = levels.submit({13, "ACME", Side::buy, 102, 4, 13});
  assert(fills.size() == 2 && fills[0].maker_id == 11 && fills[1].maker_id == 12);
#endif

#if MATCHING_STEP >= 4
  matching::OrderBook lifecycle("ACME");
  assert(lifecycle.add_resting({20, "ACME", Side::sell, 105, 2, 20}));
  const auto partial = lifecycle.submit({21, "ACME", Side::buy, 105, 5, 21,
                                         matching::TimeInForce::immediate_or_cancel});
  assert(partial.size() == 1 && !lifecycle.best_bid().has_value());
  assert(!lifecycle.cancel(20));
#endif

#if MATCHING_STEP >= 5
  matching::MatchingEngine engine;
  engine.prepare_symbol("ACME");
  assert(engine.submit({30, "ACME", Side::sell, 100, 1, 0}).empty());
  const auto engine_fills = engine.submit({31, "ACME", Side::buy, 100, 1, 0});
  assert(engine_fills.size() == 1 && engine_fills[0].sequence > 0);
#endif

#if MATCHING_STEP >= 6
  const auto first = matching::generate_mock_orders(1000, 42);
  const auto second = matching::generate_mock_orders(1000, 42);
  assert(first.size() == 1000 && first == second);
#endif

#if MATCHING_STEP >= 7
  matching::LockFreeIngress<Order> queue(1024);
  assert(queue.try_push({40, "ACME", Side::buy, 100, 1, 40}));
  const auto popped = queue.try_pop();
  assert(popped.has_value() && popped->id == 40);
#endif

#if MATCHING_STEP >= 8
  matching::MatchingEngine pipelined;
  pipelined.prepare_symbol("PIPE");
  matching::LockFreeIngress<Order> pipeline(8);
  assert(pipeline.try_push({50, "PIPE", Side::sell, 200, 1, 50}));
  assert(pipeline.try_push({51, "PIPE", Side::buy, 200, 1, 51}));
  assert(pipelined.submit(std::move(*pipeline.try_pop())).empty());
  assert(pipelined.submit(std::move(*pipeline.try_pop())).size() == 1);
#endif

#if MATCHING_STEP >= 9
  const matching::Fill event{60, 59, Side::buy, 12345, 7, 60};
  const auto envelope = matching::serialize_fill_v1(event);
  assert(envelope == "{\"version\":\"fills.v1\",\"taker_id\":60,\"maker_id\":59,\"side\":\"buy\",\"price_ticks\":12345,\"quantity\":7,\"sequence\":60}");
#endif

  std::cout << "step " << MATCHING_STEP << " gate: PASS\n";
}
