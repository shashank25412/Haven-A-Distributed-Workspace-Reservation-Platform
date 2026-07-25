/**
 * @file reservation_conflict_policy.cpp
 * @brief Implements reservation conflict evaluation.
 */

#include "haven/domain/policies/reservation_conflict_policy.hpp"

#include "haven/logging/logging.hpp"

namespace haven::domain {

bool blocks_schedule(const ReservationStatus status) noexcept {
    return status == ReservationStatus::Confirmed;
}

bool has_reservation_conflict(
    const TimeInterval& requested_interval,
    const TimeInterval& existing_interval,
    const ReservationStatus existing_status) noexcept {
    HVN_TRACE_SCOPE();

    const bool conflict = blocks_schedule(existing_status) && requested_interval.overlaps(existing_interval);

    if (conflict) {
        HVN_DEBUG_LOG("Reservation conflict detected against a confirmed interval.");
    }

    return conflict;
}

}  // namespace haven::domain