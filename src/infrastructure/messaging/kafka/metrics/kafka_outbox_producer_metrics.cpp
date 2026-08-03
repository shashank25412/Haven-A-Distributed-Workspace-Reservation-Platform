/**
 * @file kafka_outbox_producer_metrics.cpp
 * @brief Implements the bounded Kafka Outbox producer metrics catalog.
 */
#include "haven/infrastructure/messaging/kafka/metrics/kafka_outbox_producer_metrics.hpp"

#include <string>

namespace haven::infrastructure::messaging::kafka::metrics {

const application::observability::metrics::MetricName& attempts_metric_name() {
    static const auto name = application::observability::metrics::MetricName{
        "haven_kafka_outbox_publish_attempts_total"};
    return name;
}

const application::observability::metrics::MetricName& duration_metric_name() {
    static const auto name = application::observability::metrics::MetricName{
        "haven_kafka_outbox_publish_duration_seconds"};
    return name;
}

const application::observability::metrics::MetricName& payload_bytes_metric_name() {
    static const auto name =
        application::observability::metrics::MetricName{"haven_kafka_outbox_payload_bytes_total"};
    return name;
}

std::string_view outcome_value(const PublishOutcome outcome) noexcept {
    switch (outcome) {
        case PublishOutcome::acknowledged:
            return "acknowledged";
        case PublishOutcome::enqueue_failed:
            return "enqueue_failed";
        case PublishOutcome::delivery_failed:
            return "delivery_failed";
        case PublishOutcome::ack_timeout:
            return "ack_timeout";
        case PublishOutcome::unexpected_failure:
            return "unexpected_failure";
    }
    return "unexpected_failure";
}

application::observability::metrics::MetricLabel outcome_label(const PublishOutcome outcome) {
    return {"outcome", std::string{outcome_value(outcome)}};
}

}  // namespace haven::infrastructure::messaging::kafka::metrics
