#include "matching/engine.hpp"
#include "matching/ingress_queue.hpp"
#include "matching/mock_data.hpp"

#include <atomic>
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

namespace {
void unit() {
  matching::OrderBook book("TEST");
  std::vector<matching::Fill> fills;
  assert(book.submit({1, "TEST", matching::Side::sell, 101, 5, 1}, fills).accepted);
  assert(book.submit({2, "TEST", matching::Side::sell, 100, 4, 2}, fills).accepted);
  const auto ack = book.submit({3, "TEST", matching::Side::buy, 101, 7, 3}, fills);
  assert(ack.accepted && ack.remaining == 0 && fills.size() == 2);
  assert(fills[0].maker_id == 2 && fills[0].price_ticks == 100 && fills[0].quantity == 4);
  assert(fills[1].maker_id == 1 && fills[1].price_ticks == 101 && fills[1].quantity == 3);
  assert(book.best_ask() == 101);
  assert(book.cancel(1));
  assert(!book.best_ask().has_value());

  matching::OrderBook fifo("FIFO");
  std::vector<matching::Fill> fifo_fills;
  assert(fifo.submit({10, "FIFO", matching::Side::sell, 200, 2, 10}, fifo_fills).accepted);
  assert(fifo.submit({11, "FIFO", matching::Side::sell, 200, 2, 11}, fifo_fills).accepted);
  const auto fifo_ack = fifo.submit({12, "FIFO", matching::Side::buy, 200, 3, 12}, fifo_fills);
  assert(fifo_ack.accepted && fifo_ack.remaining == 0);
  assert(fifo_fills[0].maker_id == 10 && fifo_fills[1].maker_id == 11);

  matching::OrderBook ioc("IOC");
  std::vector<matching::Fill> ioc_fills;
  assert(ioc.submit({20, "IOC", matching::Side::sell, 300, 1, 20}, ioc_fills).accepted);
  const auto ioc_ack = ioc.submit({21, "IOC", matching::Side::buy, 301, 5, 21,
                                   matching::TimeInForce::immediate_or_cancel}, ioc_fills);
  assert(ioc_ack.accepted && ioc_ack.remaining == 4 && !ioc.best_bid().has_value());
}

void ingress() {
  matching::LockFreeIngress<matching::Order> queue(1024);
  constexpr std::size_t total = 50'000;
  std::atomic<std::size_t> produced{0};
  std::atomic<std::size_t> consumed{0};
  std::thread producer([&] { for (std::size_t i = 0; i < total; ++i) { matching::Order order{i + 1U, "TEST", matching::Side::buy, 100, 1, i + 1U}; while (!queue.try_push(std::move(order))) std::this_thread::yield(); produced.fetch_add(1); } });
  std::thread consumer([&] { while (consumed.load() < total) if (queue.try_pop()) consumed.fetch_add(1); else std::this_thread::yield(); });
  producer.join(); consumer.join();
  assert(produced == total && consumed == total);
}

void mock() {
  matching::MockDataConfig config; config.count = 1000; config.seed = 42;
  const auto first = matching::generate_mock_orders(config);
  const auto second = matching::generate_mock_orders(config);
  assert(first.size() == second.size());
  for (std::size_t i = 0; i < first.size(); ++i) assert(first[i].id == second[i].id && first[i].price_ticks == second[i].price_ticks && first[i].side == second[i].side);
}
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string suite = argv[1];
  if (suite == "unit") unit(); else if (suite == "ingress") ingress(); else if (suite == "mock") mock(); else return 2;
  std::cout << suite << ": PASS\n";
}
