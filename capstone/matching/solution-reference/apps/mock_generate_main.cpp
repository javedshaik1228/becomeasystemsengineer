// Optional completed reference. Build in capstone/matching/work first.
#include "matching/mock_data.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
  const std::string output = argc > 1 ? argv[1] : "mock-orders.csv";
  matching::MockDataConfig config;
  if (argc > 2) config.count = std::strtoull(argv[2], nullptr, 10);
  if (argc > 3) config.seed = std::strtoull(argv[3], nullptr, 10);
  const auto orders = matching::generate_mock_orders(config);
  if (!matching::write_mock_orders(output, orders)) { std::cerr << "could not write " << output << '\n'; return 1; }
  std::cout << "wrote " << orders.size() << " deterministic orders to " << output << " seed=" << config.seed << '\n';
}
