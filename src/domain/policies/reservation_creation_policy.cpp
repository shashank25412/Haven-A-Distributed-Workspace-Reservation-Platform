/**
 * @file reservation_creation_policy.cpp
 * @brief Implements reservation creation decision evaluation.
 */

#include "haven/domain/policies/reservation_creation_policy.hpp"

#include "haven/logging/logging.hpp"

namespace haven::domain {
namespace {

ReservationCreationDecision map_violation(const ReservationPolicyViolation violation) noexcept {
    switch (violation) {
        case ReservationPolicyViolation::None:
            break;
        case ReservationPolicyViolation::MaintenanceAuthorizationRequired:
            return ReservationCreationDecision::MaintenanceAuthorizationRequired;
        case ReservationPolicyViolation::StandardDurationExceeded:
            return ReservationCreationDecision::StandardDurationExceeded;
        case ReservationPolicyViolation::MaintenanceDurationExceeded:
            return ReservationCreationDecision::MaintenanceDurationExceeded;
    }

    return ReservationCreationDecision::Confirmed;
}

}  // namespace

ReservationCreationDecision ReservationCreationPolicy::evaluate(
    const Resource& resource,
    const TimeInterval& interval,
    const ReservationKind kind,
    const bool maintenance_authorized) const noexcept {
    HVN_TRACE_SCOPE();

    if (!resource.is_active()) {
        HVN_WARN_LOG("Reservation creation denied because the resource is inactive.");
        return ReservationCreationDecision::ResourceInactive;
    }

    const ReservationPolicyViolation violation =
        reservation_policy_.evaluate(interval, kind, maintenance_authorized);

    if (violation != ReservationPolicyViolation::None) {
        HVN_WARN_LOG("Reservation creation denied because a reservation policy was violated.");
        return map_violation(violation);
    }

    if (resource.requires_approval()) {
        HVN_DEBUG_LOG("Reservation creation requires approval.");
        return ReservationCreationDecision::PendingApproval;
    }

    HVN_DEBUG_LOG("Reservation creation may proceed as confirmed.");
    return ReservationCreationDecision::Confirmed;
}

}  // namespace haven::domain