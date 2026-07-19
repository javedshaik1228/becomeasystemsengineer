// Optional completed reference. Build in capstone/matching/work first.
#include "matching/kafka_edge.hpp"

#include <sstream>

#ifdef MATCHING_WITH_RDKAFKA
#include <librdkafka/rdkafka.h>
#endif

namespace matching {

KafkaProducer::KafkaProducer(std::string brokers, std::string topic)
    : brokers_(std::move(brokers)), topic_(std::move(topic)) {
#ifdef MATCHING_WITH_RDKAFKA
  char error[512]{};
  auto* conf = rd_kafka_conf_new();
  if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers_.c_str(), error, sizeof(error)) == RD_KAFKA_CONF_OK) {
    producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, error, sizeof(error));
    if (producer_ != nullptr) {
      topic_handle_ = rd_kafka_topic_new(static_cast<rd_kafka_t*>(producer_), topic_.c_str(), nullptr);
    }
  } else {
    rd_kafka_conf_destroy(conf);
  }
#endif
}

KafkaProducer::~KafkaProducer() {
#ifdef MATCHING_WITH_RDKAFKA
  if (topic_handle_ != nullptr) rd_kafka_topic_destroy(static_cast<rd_kafka_topic_t*>(topic_handle_));
  if (producer_ != nullptr) {
    rd_kafka_flush(static_cast<rd_kafka_t*>(producer_), 1000);
    rd_kafka_destroy(static_cast<rd_kafka_t*>(producer_));
  }
#endif
}

EdgeResult KafkaProducer::publish(const Fill& fill) const {
  std::ostringstream envelope;
  envelope << "{\"topic\":\"" << topic_ << "\",\"brokers\":\"" << brokers_
           << "\",\"taker_id\":" << fill.taker_id << ",\"maker_id\":" << fill.maker_id
           << ",\"price_ticks\":" << fill.price_ticks << ",\"quantity\":" << fill.quantity << "}";
#ifdef MATCHING_WITH_RDKAFKA
  if (producer_ == nullptr || topic_handle_ == nullptr) return {false, "librdkafka initialization failed"};
  const auto payload = envelope.str();
  const auto result = rd_kafka_produce(static_cast<rd_kafka_topic_t*>(topic_handle_),
                                       RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
                                       const_cast<char*>(payload.data()), payload.size(), nullptr, 0, nullptr);
  if (result != 0) return {false, rd_kafka_err2str(rd_kafka_last_error())};
  rd_kafka_poll(static_cast<rd_kafka_t*>(producer_), 0);
  return {true, "queued"};
#else
  return {false, "Kafka edge is an explicit sidecar seam; use the envelope with kcat/librdkafka"};
#endif
}

}  // namespace matching
