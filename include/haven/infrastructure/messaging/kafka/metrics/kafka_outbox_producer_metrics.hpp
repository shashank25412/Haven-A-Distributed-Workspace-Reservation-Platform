/**
 * @file kafka_outbox_producer_metrics.hpp
 * @brief Defines the bounded Kafka Outbox producer metrics catalog.
 */
#pragma once

#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"

#include <string_view>

namespace haven::infrastructure::messaging::kafka::metrics {

/** Terminal outcomes for one Kafka producer-port invocation. */
enum class PublishOutcome {
    acknowledged,
    enqueue_failed,
    delivery_failed,
    ack_timeout,
    unexpected_failure,
};

/** Counter incremented once per producer invocation and labelled only by terminal outcome. */
[[nodiscard]] const application::observability::metrics::MetricName& attempts_metric_name();
/** Full adapter latency in seconds when exported, labelled only by terminal outcome. */
[[nodiscard]] const application::observability::metrics::MetricName& duration_metric_name();
/** Cumulative serialized envelope body bytes; keys and headers are excluded. */
[[nodiscard]] const application::observability::metrics::MetricName& payload_bytes_metric_name();

[[nodiscard]] std::string_view outcome_value(PublishOutcome outcome) noexcept;
[[nodiscard]] application::observability::metrics::MetricLabel outcome_label(
    PublishOutcome outcome);

}  // namespace haven::infrastructure::messaging::kafka::metrics
