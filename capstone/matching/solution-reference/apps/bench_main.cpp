// Optional completed reference. Build in capstone/matching/work first.
#include "matching/engine.hpp"
#include "matching/ingress_queue.hpp"
#include "matching/mock_data.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
  matching::MockDataConfig config;
  if (argc > 1) config.count = std::stoull(argv[1]);
  const auto orders = matching::generate_mock_orders(config);
  matching::LockFreeIngress<matching::Order> ingress(1U << 16U);
  matching::MatchingEngine engine;
  engine.prepare_symbol(config.symbol);
  std::vector<matching::Fill> fills;
  const auto start = std::chrono::steady_clock::now();
  std::thread producer([&] { for (const auto& order : orders) while (!ingress.try_push(order)) std::this_thread::yield(); });
  std::size_t consumed = 0;
  while (consumed < orders.size()) {
    if (auto order = ingress.try_pop()) { engine.submit(std::move(*order), fills); ++consumed; }
    else std::this_thread::yield();
  }
  producer.join();
  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  std::cout << "orders=" << consumed << " fills=" << fills.size() << " seconds=" << elapsed
            << " throughput_per_second=" << (static_cast<double>(consumed) / elapsed) << '\n';
}
