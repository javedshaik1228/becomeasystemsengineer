// Optional completed reference. Build in capstone/matching/work first.
#include "matching/order_book.hpp"

#include <algorithm>

namespace matching {

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

Ack OrderBook::submit(Order order, std::vector<Fill>& fills) {
  if (order.symbol != symbol_) return {order.id, false, 0, "symbol mismatch"};
  if (order.id == 0 || order.quantity == 0 || order.price_ticks <= 0) return {order.id, false, 0, "invalid order"};
  if (live_orders_.contains(order.id)) return {order.id, false, 0, "duplicate live order id"};

  auto remaining = order.quantity;
  const auto crosses = [&] (std::int64_t maker_price) {
    return order.side == Side::buy ? maker_price <= order.price_ticks : maker_price >= order.price_ticks;
  };
  while (remaining > 0U) {
    if (order.side == Side::buy) {
      if (asks_.empty() || !crosses(asks_.begin()->first)) break;
      auto level = asks_.begin();
      auto& maker = level->second.orders.front();
      const auto traded = std::min(remaining, maker.quantity);
      fills.push_back({order.id, maker.id, order.side, maker.price_ticks, traded, order.sequence});
      remaining -= traded;
      maker.quantity -= traded;
      if (maker.quantity == 0U) {
        live_orders_.erase(maker.id);
        level->second.orders.pop_front();
        if (level->second.orders.empty()) asks_.erase(level);
      }
    } else {
      if (bids_.empty() || !crosses(bids_.begin()->first)) break;
      auto level = bids_.begin();
      auto& maker = level->second.orders.front();
      const auto traded = std::min(remaining, maker.quantity);
      fills.push_back({order.id, maker.id, order.side, maker.price_ticks, traded, order.sequence});
      remaining -= traded;
      maker.quantity -= traded;
      if (maker.quantity == 0U) {
        live_orders_.erase(maker.id);
        level->second.orders.pop_front();
        if (level->second.orders.empty()) bids_.erase(level);
      }
    }
  }

  if (remaining > 0U && order.tif == TimeInForce::good_till_cancel) {
    order.quantity = remaining;
    const auto resting_id = order.id;
    const auto resting_side = order.side;
    if (order.side == Side::buy) bids_[order.price_ticks].orders.push_back(std::move(order));
    else asks_[order.price_ticks].orders.push_back(std::move(order));
    live_orders_.emplace(resting_id, resting_side);
  }
  return {order.id, true, remaining, "accepted"};
}

bool OrderBook::cancel(std::uint64_t order_id) {
  const auto found = live_orders_.find(order_id);
  if (found == live_orders_.end()) return false;
  if (found->second == Side::buy) {
    for (auto level = bids_.begin(); level != bids_.end(); ++level) {
      auto& orders = level->second.orders;
      const auto item = std::find_if(orders.begin(), orders.end(), [order_id](const Order& order) { return order.id == order_id; });
      if (item != orders.end()) {
        orders.erase(item);
        if (orders.empty()) bids_.erase(level);
        live_orders_.erase(found);
        return true;
      }
    }
  } else {
    for (auto level = asks_.begin(); level != asks_.end(); ++level) {
      auto& orders = level->second.orders;
      const auto item = std::find_if(orders.begin(), orders.end(), [order_id](const Order& order) { return order.id == order_id; });
      if (item != orders.end()) {
        orders.erase(item);
        if (orders.empty()) asks_.erase(level);
        live_orders_.erase(found);
        return true;
      }
    }
  }
  return false;
}

std::optional<std::int64_t> OrderBook::best_bid() const { return bids_.empty() ? std::nullopt : std::optional{bids_.begin()->first}; }
std::optional<std::int64_t> OrderBook::best_ask() const { return asks_.empty() ? std::nullopt : std::optional{asks_.begin()->first}; }
void OrderBook::erase_empty_levels() {}

}  // namespace matching
