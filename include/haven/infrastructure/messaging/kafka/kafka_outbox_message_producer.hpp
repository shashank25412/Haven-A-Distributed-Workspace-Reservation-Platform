/**
 * @file kafka_outbox_message_producer.hpp
 * @brief Declares the acknowledged librdkafka Outbox producer adapter.
 */
#pragma once

#include "haven/application/observability/metrics/metrics_recorder.hpp"
#include "haven/application/outbox/outbox_message_producer.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_producer_configuration.hpp"
#include "haven/infrastructure/messaging/kafka/metrics/kafka_outbox_producer_metrics.hpp"

#include <chrono>
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

    KafkaOutboxMessageProducer(
        KafkaProducerConfiguration configuration,
        haven::application::observability::metrics::MetricsRecorder& metrics_recorder);
    ~KafkaOutboxMessageProducer() override;

    KafkaOutboxMessageProducer(const KafkaOutboxMessageProducer&) = delete;
    KafkaOutboxMessageProducer& operator=(const KafkaOutboxMessageProducer&) = delete;
    KafkaOutboxMessageProducer(KafkaOutboxMessageProducer&&) = delete;
    KafkaOutboxMessageProducer& operator=(KafkaOutboxMessageProducer&&) = delete;

    void publish(const haven::application::outbox::OutboxMessage& message) override;

private:
    void publish_record(const haven::application::outbox::OutboxMessage& message,
                        metrics::PublishOutcome& outcome);
    void record_terminal(metrics::PublishOutcome outcome,
                         std::chrono::steady_clock::time_point started_at) noexcept;

    KafkaProducerConfiguration configuration_;
    std::unique_ptr<rd_kafka_t, ProducerDeleter> producer_;
    haven::application::observability::metrics::MetricsRecorder& metrics_recorder_;
    std::vector<std::shared_ptr<void>> timed_out_delivery_states_;
    std::mutex mutex_;
};

}  // namespace haven::infrastructure::messaging::kafka
