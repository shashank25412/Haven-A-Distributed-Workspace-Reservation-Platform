/**
 * @file reservation_creation_policy.hpp
 * @brief Defines reservation creation decision evaluation.
 */

#pragma once

#include "haven/domain/policies/reservation_policy.hpp"
#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

namespace haven::domain {

/**
 * @brief Represents the outcome of reservation creation policy evaluation.
 *
 * Successful decisions identify the initial reservation state. Rejected
 * decisions describe the violated business rule.
 */
enum class ReservationCreationDecision {
    Confirmed,
    PendingApproval,
    ResourceInactive,
    MaintenanceAuthorizationRequired,
    StandardDurationExceeded,
    MaintenanceDurationExceeded
};

/**
 * @brief Determines whether a reservation may be created for a resource.
 *
 * Evaluation combines resource activity, reservation duration, maintenance
 * authorization, and the resource approval requirement. Conflict checking is
 * intentionally handled separately against authoritative reservation state.
 */
class ReservationCreationPolicy final {
public:
    /**
     * @brief Evaluates a reservation creation request.
     *
     * @param resource Resource requested by the caller.
     * @param interval Requested reservation interval.
     * @param kind Requested reservation kind.
     * @param maintenance_authorized Whether maintenance creation is authorized.
     *
     * @return Reservation creation decision.
     */
    [[nodiscard]] ReservationCreationDecision evaluate(
        const Resource& resource,
        const TimeInterval& interval,
        ReservationKind kind,
        bool maintenance_authorized) const noexcept;

private:
    ReservationPolicy reservation_policy_;
};

}  // namespace haven::domain