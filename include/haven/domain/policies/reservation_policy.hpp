/**
 * @file reservation_policy.hpp
 * @brief Defines reservation creation policy evaluation.
 */

#pragma once

#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

#include <chrono>

namespace haven::domain {

/**
 * @brief Identifies why a reservation violates the creation policy.
 */
enum class ReservationPolicyViolation {
    None,
    MaintenanceAuthorizationRequired,
    StandardDurationExceeded,
    MaintenanceDurationExceeded
};

/**
 * @brief Evaluates reservation duration and authorization rules.
 *
 * Standard reservations may last at most 12 hours. Authorized maintenance
 * reservations may last at most 24 hours. Maintenance authorization is
 * supplied explicitly and is never inferred from free-form purpose text.
 */
class ReservationPolicy final {
public:
    static constexpr std::chrono::hours kMaximumStandardDuration{12};
    static constexpr std::chrono::hours kMaximumMaintenanceDuration{24};

    /**
     * @brief Evaluates whether a reservation satisfies creation policy.
     *
     * @param interval Requested reservation interval.
     * @param kind Requested reservation kind.
     * @param maintenance_authorized Whether the caller may create maintenance reservations.
     *
     * @return The first applicable policy violation, or None when allowed.
     */
    [[nodiscard]] ReservationPolicyViolation evaluate(
        const TimeInterval& interval,
        ReservationKind kind,
        bool maintenance_authorized) const noexcept;
};

}  // namespace haven::domain