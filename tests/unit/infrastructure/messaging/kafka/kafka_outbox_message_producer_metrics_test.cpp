/**
 * @file kafka_outbox_message_producer_metrics_test.cpp
 * @brief Tests broker-independent Kafka producer metric behavior.
 */
#include "haven/application/outbox/message_publish_error.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_outbox_message_producer.hpp"

#include "application/util/recording_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::infrastructure::messaging::kafka {
namespace {
using RecordingMetrics = haven::test::application::observability::metrics::RecordingMetricsRecorder;

KafkaProducerConfiguration configuration() {
    return {.enabled = true,
            .brokers = "127.0.0.1:1",
            .reservation_events_topic = "haven.test",
            .client_id = "haven-metrics-test",
            .acknowledgement_timeout = std::chrono::milliseconds{1},
            .delivery_timeout = std::chrono::milliseconds{1}};
}

haven::application::outbox::OutboxMessage empty_message() {
    return {.event_id = haven::domain::EventId{"event"},
            .organization_id = haven::domain::OrganizationId{"organization"},
            .aggregate_id = haven::domain::ReservationId{"reservation"},
            .aggregate_type = "Reservation",
            .event_type = "ReservationCreated",
            .occurred_at = {},
            .schema_version = 1,
            .serialized_envelope = {},
            .attempt_count = 0};
}

class ThrowingMetrics final : public application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(const application::observability::metrics::MetricName&,
                           double,
                           const application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
    void set_gauge(const application::observability::metrics::MetricName&,
                   double,
                   const application::observability::metrics::MetricLabels&) override {}
    void observe_duration(const application::observability::metrics::MetricName&,
                          std::chrono::microseconds,
                          const application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
};
}  // namespace

TEST(KafkaOutboxMessageProducerMetricsTest, LocalRecordFailureHasOneTerminalOutcome) {
    auto metrics = RecordingMetrics{};
    auto producer = KafkaOutboxMessageProducer{configuration(), metrics};

    EXPECT_THROW(producer.publish(empty_message()), application::outbox::MessagePublishError);

    ASSERT_EQ(metrics.counter_increments().size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].name.value(),
              "haven_kafka_outbox_publish_attempts_total");
    EXPECT_EQ(metrics.counter_increments()[0].amount, 1.0);
    ASSERT_EQ(metrics.counter_increments()[0].labels.size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].labels[0].value(), "enqueue_failed");
    ASSERT_EQ(metrics.duration_observations().size(), 1U);
    EXPECT_EQ(metrics.duration_observations()[0].labels[0].value(), "enqueue_failed");
    EXPECT_GE(metrics.duration_observations()[0].duration, std::chrono::microseconds::zero());
}

TEST(KafkaOutboxMessageProducerMetricsTest, MetricsFailuresPreserveOriginalProducerError) {
    auto metrics = ThrowingMetrics{};
    auto producer = KafkaOutboxMessageProducer{configuration(), metrics};

    try {
        producer.publish(empty_message());
        FAIL() << "Expected local record validation failure";
    } catch (const application::outbox::MessagePublishError& error) {
        EXPECT_EQ(error.code(), application::outbox::MessagePublishErrorCode::InvalidMessage);
    }
}

}  // namespace haven::infrastructure::messaging::kafka
