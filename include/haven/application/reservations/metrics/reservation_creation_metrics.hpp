/**
 * @file reservation_creation_metrics.hpp
 * @brief Defines the bounded reservation-creation metrics catalog.
 */

#pragma once

#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"

#include <string_view>

namespace haven::application::reservations::metrics {

/**
 * @brief Stable, low-cardinality reservation creation outcomes.
 *
 * Both metrics use only the `outcome` label with one of these values. Caller,
 * tenant, resource, reservation, event, and idempotency identifiers are never labels.
 */
enum class ReservationCreationOutcome {
    created_confirmed,
    created_pending_approval,
    replayed,
    idempotency_in_progress,
    idempotency_mismatch,
    resource_not_found,
    resource_inactive,
    reservation_conflict,
    validation_failed,
    persistence_failed,
    unexpected_failure,
};

/** Counter incremented once for each completed handler invocation. */
[[nodiscard]] const observability::metrics::MetricName& attempts_metric_name();

/** Summary observation containing full handler latency in seconds when exported. */
[[nodiscard]] const observability::metrics::MetricName& duration_metric_name();

[[nodiscard]] std::string_view outcome_value(ReservationCreationOutcome outcome) noexcept;
[[nodiscard]] observability::metrics::MetricLabel outcome_label(ReservationCreationOutcome outcome);

}  // namespace haven::application::reservations::metrics
