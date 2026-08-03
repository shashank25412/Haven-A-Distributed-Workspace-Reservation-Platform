/**
 * @file kafka_producer_configuration.hpp
 * @brief Defines validated Kafka producer settings.
 */
#pragma once

#include <chrono>
#include <string>

namespace haven::infrastructure::messaging::kafka {

struct KafkaProducerConfiguration final {
    bool enabled;
    std::string brokers;
    std::string reservation_events_topic;
    std::string client_id;
    std::chrono::milliseconds acknowledgement_timeout;
    std::chrono::milliseconds delivery_timeout;
};

}  // namespace haven::infrastructure::messaging::kafka
