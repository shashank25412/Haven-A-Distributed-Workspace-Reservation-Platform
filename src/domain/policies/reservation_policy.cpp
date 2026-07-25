/**
 * @file reservation_policy.cpp
 * @brief Implements reservation creation policy evaluation.
 */

#include "haven/domain/policies/reservation_policy.hpp"

namespace haven::domain {

ReservationPolicyViolation ReservationPolicy::evaluate(
    const TimeInterval& interval,
    const ReservationKind kind,
    const bool maintenance_authorized) const noexcept {
    if (kind == ReservationKind::Maintenance) {
        if (!maintenance_authorized) {
            return ReservationPolicyViolation::MaintenanceAuthorizationRequired;
        }

        if (interval.duration() > kMaximumMaintenanceDuration) {
            return ReservationPolicyViolation::MaintenanceDurationExceeded;
        }

        return ReservationPolicyViolation::None;
    }

    if (interval.duration() > kMaximumStandardDuration) {
        return ReservationPolicyViolation::StandardDurationExceeded;
    }

    return ReservationPolicyViolation::None;
}

}  // namespace haven::domain