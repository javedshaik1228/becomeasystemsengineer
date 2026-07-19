#include "matching/mock_data.hpp"

#include <fstream>
#include <sstream>

namespace matching {
namespace {
class Rng {
 public:
  explicit Rng(std::uint64_t seed) : state_(seed == 0 ? 1 : seed) {}
  std::uint64_t next() noexcept { state_ ^= state_ << 7U; state_ ^= state_ >> 9U; state_ ^= state_ << 8U; return state_; }
 private:
  std::uint64_t state_;
};
}  // namespace

std::vector<Order> generate_mock_orders(const MockDataConfig& config) {
  Rng rng(config.seed);
  std::vector<Order> orders;
  orders.reserve(config.count);
  const auto band = static_cast<std::uint64_t>(config.price_band_ticks > 0 ? config.price_band_ticks : 1);
  for (std::size_t index = 0; index < config.count; ++index) {
    const auto offset = static_cast<std::int64_t>(rng.next() % (2ULL * band + 1ULL)) - static_cast<std::int64_t>(band);
    const auto side = (rng.next() & 1U) == 0U ? Side::buy : Side::sell;
    orders.push_back({static_cast<std::uint64_t>(index + 1U), config.symbol, side,
                      config.center_price_ticks + offset, (rng.next() % 100U) + 1U,
                      static_cast<std::uint64_t>(index + 1U), TimeInForce::good_till_cancel});
  }
  return orders;
}

bool write_mock_orders(const std::string& path, const std::vector<Order>& orders) {
  std::ofstream output(path);
  if (!output) return false;
  output << "id,symbol,side,price_ticks,quantity,sequence,tif\n";
  for (const auto& order : orders) {
    output << order.id << ',' << order.symbol << ',' << (order.side == Side::buy ? 'B' : 'S') << ','
           << order.price_ticks << ',' << order.quantity << ',' << order.sequence << ','
           << (order.tif == TimeInForce::good_till_cancel ? 'G' : 'I') << '\n';
  }
  return static_cast<bool>(output);
}

std::vector<Order> read_mock_orders(const std::string& path) {
  std::ifstream input(path);
  std::vector<Order> orders;
  std::string line;
  std::getline(input, line);
  while (std::getline(input, line)) {
    Order order;
    char side = 'B'; char tif = 'G';
    std::stringstream parser(line);
    std::string field;
    std::getline(parser, field, ','); order.id = std::stoull(field);
    std::getline(parser, order.symbol, ',');
    std::getline(parser, field, ','); side = field.front(); order.side = side == 'B' ? Side::buy : Side::sell;
    std::getline(parser, field, ','); order.price_ticks = std::stoll(field);
    std::getline(parser, field, ','); order.quantity = std::stoull(field);
    std::getline(parser, field, ','); order.sequence = std::stoull(field);
    std::getline(parser, field, ','); tif = field.front(); order.tif = tif == 'I' ? TimeInForce::immediate_or_cancel : TimeInForce::good_till_cancel;
    orders.push_back(std::move(order));
  }
  return orders;
}

}  // namespace matching
