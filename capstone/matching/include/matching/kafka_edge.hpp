#pragma once

#include "matching/order.hpp"
#include "matching/redis_edge.hpp"

#include <cstdint>
#include <string>

namespace matching {

// Kafka edge contract. The default build is dependency-free and emits a
// deterministic envelope for an external producer (kcat/librdkafka/sidecar).
// Define MATCHING_WITH_RDKAFKA in a deployment build to replace this seam.
class KafkaProducer {
 public:
  KafkaProducer(std::string brokers, std::string topic);
  ~KafkaProducer();
  KafkaProducer(const KafkaProducer&) = delete;
  KafkaProducer& operator=(const KafkaProducer&) = delete;
  [[nodiscard]] EdgeResult publish(const Fill& fill) const;

 private:
  std::string brokers_;
  std::string topic_;
  void* producer_{nullptr};
  void* topic_handle_{nullptr};
};

}  // namespace matching
