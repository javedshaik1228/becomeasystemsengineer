#pragma once

#include "matching/order.hpp"

#include <string>
#include <string_view>

namespace matching {

class FillSink {
 public:
  virtual ~FillSink() = default;
  virtual bool publish(std::string_view payload) = 0;
};

// Step 9 TODO: produce a stable fills.v1 envelope before wiring Redis/Kafka adapters.
[[nodiscard]] std::string serialize_fill_v1(const Fill& fill);

}  // namespace matching
