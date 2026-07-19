#include "matching/order_book.hpp"

#include <utility>

namespace matching {

bool valid_order(const Order&) noexcept { return false; }

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}
bool OrderBook::add_resting(Order) { return false; }
std::vector<Fill> OrderBook::submit(Order) { return {}; }
bool OrderBook::cancel(std::uint64_t) { return false; }
std::optional<std::int64_t> OrderBook::best_bid() const { return std::nullopt; }
std::optional<std::int64_t> OrderBook::best_ask() const { return std::nullopt; }
std::size_t OrderBook::resting_orders() const noexcept { return 0; }

}  // namespace matching
