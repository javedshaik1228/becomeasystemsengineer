#pragma once

#include "matching/order.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace matching {

struct MockDataConfig {
  std::size_t count{100'000};
  std::uint64_t seed{0x5eedU};
  std::string symbol{"NETF"};
  std::int64_t center_price_ticks{10'000};
  std::int64_t price_band_ticks{500};
};

[[nodiscard]] std::vector<Order> generate_mock_orders(const MockDataConfig& config);
bool write_mock_orders(const std::string& path, const std::vector<Order>& orders);
[[nodiscard]] std::vector<Order> read_mock_orders(const std::string& path);

}  // namespace matching

