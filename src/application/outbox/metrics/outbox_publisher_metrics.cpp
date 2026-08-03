/**
 * @file outbox_publisher_metrics.cpp
 * @brief Implements the bounded Outbox publisher metrics catalog.
 */
#include "haven/application/outbox/metrics/outbox_publisher_metrics.hpp"

#include <string>

namespace haven::application::outbox::metrics {

const observability::metrics::MetricName& cycles_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_outbox_publisher_cycles_total"};
    return name;
}
const observability::metrics::MetricName& cycle_duration_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_outbox_publisher_cycle_duration_seconds"};
    return name;
}
const observability::metrics::MetricName& records_discovered_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_outbox_publisher_records_discovered_total"};
    return name;
}
const observability::metrics::MetricName& record_attempts_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_outbox_publisher_record_attempts_total"};
    return name;
}
const observability::metrics::MetricName& worker_running_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_outbox_publisher_worker_running"};
    return name;
}

std::string_view outcome_value(const CycleOutcome outcome) noexcept {
    switch (outcome) {
        case CycleOutcome::completed:
            return "completed";
        case CycleOutcome::repository_failure:
            return "repository_failure";
        case CycleOutcome::unexpected_failure:
            return "unexpected_failure";
    }
    return "unexpected_failure";
}

std::string_view outcome_value(const RecordOutcome outcome) noexcept {
    switch (outcome) {
        case RecordOutcome::published:
            return "published";
        case RecordOutcome::claim_not_acquired:
            return "claim_not_acquired";
        case RecordOutcome::claim_failed:
            return "claim_failed";
        case RecordOutcome::publish_failed_released:
            return "publish_failed_released";
        case RecordOutcome::publish_failed_release_failed:
            return "publish_failed_release_failed";
        case RecordOutcome::published_mark_failed:
            return "published_mark_failed";
        case RecordOutcome::record_failed:
            return "record_failed";
    }
    return "record_failed";
}

observability::metrics::MetricLabel outcome_label(const CycleOutcome outcome) {
    return {"outcome", std::string{outcome_value(outcome)}};
}
observability::metrics::MetricLabel outcome_label(const RecordOutcome outcome) {
    return {"outcome", std::string{outcome_value(outcome)}};
}

}  // namespace haven::application::outbox::metrics
