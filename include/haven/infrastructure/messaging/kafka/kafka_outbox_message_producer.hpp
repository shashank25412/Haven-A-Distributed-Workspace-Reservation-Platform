/**
 * @file kafka_outbox_message_producer.hpp
 * @brief Declares the acknowledged librdkafka Outbox producer adapter.
 */
#pragma once

#include "haven/application/outbox/outbox_message_producer.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_producer_configuration.hpp"

#include <librdkafka/rdkafka.h>
#include <memory>
#include <mutex>
#include <vector>

namespace haven::infrastructure::messaging::kafka {

/**
 * @brief Long-lived synchronous Kafka producer.
 *
 * Publish calls and destruction are serialized so each delivery report has one coordinated owner.
 */
class KafkaOutboxMessageProducer final : public haven::application::outbox::OutboxMessageProducer {
public:
    struct ProducerDeleter {
        void operator()(rd_kafka_t* producer) const noexcept;
    };

    explicit KafkaOutboxMessageProducer(KafkaProducerConfiguration configuration);
    ~KafkaOutboxMessageProducer() override;

    KafkaOutboxMessageProducer(const KafkaOutboxMessageProducer&) = delete;
    KafkaOutboxMessageProducer& operator=(const KafkaOutboxMessageProducer&) = delete;
    KafkaOutboxMessageProducer(KafkaOutboxMessageProducer&&) = delete;
    KafkaOutboxMessageProducer& operator=(KafkaOutboxMessageProducer&&) = delete;

    void publish(const haven::application::outbox::OutboxMessage& message) override;

private:
    KafkaProducerConfiguration configuration_;
    std::unique_ptr<rd_kafka_t, ProducerDeleter> producer_;
    std::vector<std::shared_ptr<void>> timed_out_delivery_states_;
    std::mutex mutex_;
};

}  // namespace haven::infrastructure::messaging::kafka
