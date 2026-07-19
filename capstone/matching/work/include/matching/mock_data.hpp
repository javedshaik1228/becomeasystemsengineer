#pragma once

#include "matching/order.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace matching {

// Step 6 TODO: make identical seeds produce byte-for-byte equivalent streams.
[[nodiscard]] std::vector<Order> generate_mock_orders(std::size_t count, std::uint64_t seed);

}  // namespace matching
