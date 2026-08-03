/**
 * @file reservation_creation_metrics.cpp
 * @brief Implements the bounded reservation-creation metrics catalog.
 */

#include "haven/application/reservations/metrics/reservation_creation_metrics.hpp"

#include <string>

namespace haven::application::reservations::metrics {

const observability::metrics::MetricName& attempts_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_reservation_creation_attempts_total"};
    return name;
}

const observability::metrics::MetricName& duration_metric_name() {
    static const auto name =
        observability::metrics::MetricName{"haven_reservation_creation_duration_seconds"};
    return name;
}

std::string_view outcome_value(const ReservationCreationOutcome outcome) noexcept {
    switch (outcome) {
        case ReservationCreationOutcome::created_confirmed:
            return "created_confirmed";
        case ReservationCreationOutcome::created_pending_approval:
            return "created_pending_approval";
        case ReservationCreationOutcome::replayed:
            return "replayed";
        case ReservationCreationOutcome::idempotency_in_progress:
            return "idempotency_in_progress";
        case ReservationCreationOutcome::idempotency_mismatch:
            return "idempotency_mismatch";
        case ReservationCreationOutcome::resource_not_found:
            return "resource_not_found";
        case ReservationCreationOutcome::resource_inactive:
            return "resource_inactive";
        case ReservationCreationOutcome::reservation_conflict:
            return "reservation_conflict";
        case ReservationCreationOutcome::validation_failed:
            return "validation_failed";
        case ReservationCreationOutcome::persistence_failed:
            return "persistence_failed";
        case ReservationCreationOutcome::unexpected_failure:
            return "unexpected_failure";
    }
    return "unexpected_failure";
}

observability::metrics::MetricLabel outcome_label(const ReservationCreationOutcome outcome) {
    return observability::metrics::MetricLabel{"outcome", std::string{outcome_value(outcome)}};
}

}  // namespace haven::application::reservations::metrics
