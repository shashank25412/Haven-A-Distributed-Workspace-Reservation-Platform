/**
 * @file kafka_outbox_record.hpp
 * @brief Declares pure mapping from Outbox messages to Kafka records.
 */
#pragma once

#include "haven/application/outbox/outbox_message.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_producer_configuration.hpp"

#include <string>
#include <utility>
#include <vector>

namespace haven::infrastructure::messaging::kafka {

struct KafkaOutboxRecord final {
    std::string topic;
    std::string key;
    std::string payload;
    std::vector<std::pair<std::string, std::string>> headers;
};

[[nodiscard]] KafkaOutboxRecord to_kafka_outbox_record(
    const haven::application::outbox::OutboxMessage& message,
    const KafkaProducerConfiguration& configuration);

}  // namespace haven::infrastructure::messaging::kafka
