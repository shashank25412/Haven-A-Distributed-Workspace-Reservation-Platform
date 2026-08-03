/**
 * @file outbox_publisher_metrics.hpp
 * @brief Defines the bounded Outbox publisher metrics catalog.
 */
#pragma once

#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"

#include <string_view>

namespace haven::application::outbox::metrics {

/** Stable outcomes recorded exactly once for each invoked publisher cycle. */
enum class CycleOutcome { completed, repository_failure, unexpected_failure };

/**
 * Stable outcomes recorded once for each discovered record that enters processing.
 * `published_mark_failed` means the broker acknowledged publication but persistence did not
 * confirm completion, so a later retry can publish the message again.
 */
enum class RecordOutcome {
    published,
    claim_not_acquired,
    claim_failed,
    publish_failed_released,
    publish_failed_release_failed,
    published_mark_failed,
    record_failed,
};

/** Counter incremented once per cycle and labelled only by cycle outcome. */
[[nodiscard]] const observability::metrics::MetricName& cycles_metric_name();
/** Full cycle latency in seconds when exported, labelled only by cycle outcome. */
[[nodiscard]] const observability::metrics::MetricName& cycle_duration_metric_name();
/** Counter incremented by the pending-query result size, including explicit zero increments. */
[[nodiscard]] const observability::metrics::MetricName& records_discovered_metric_name();
/** Counter incremented once per processed record and labelled only by record outcome. */
[[nodiscard]] const observability::metrics::MetricName& record_attempts_metric_name();
/** Gauge set to one inside the worker lifecycle and zero when it exits or fails to start. */
[[nodiscard]] const observability::metrics::MetricName& worker_running_metric_name();

[[nodiscard]] std::string_view outcome_value(CycleOutcome outcome) noexcept;
[[nodiscard]] std::string_view outcome_value(RecordOutcome outcome) noexcept;
[[nodiscard]] observability::metrics::MetricLabel outcome_label(CycleOutcome outcome);
[[nodiscard]] observability::metrics::MetricLabel outcome_label(RecordOutcome outcome);

}  // namespace haven::application::outbox::metrics
