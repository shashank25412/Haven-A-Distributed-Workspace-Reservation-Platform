/**
 * @file approve_reservation_command.hpp
 * @brief Defines the input command for approving a reservation.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a request to approve a pending reservation.
 *
 * The approver identity, event identifier, and timestamp are supplied by the
 * caller so the handler remains deterministic and independently testable.
 */
class ApproveReservationCommand final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation approval command.
     *
     * @param organization_id Organization used to scope the operation.
     * @param reservation_id Reservation requested for approval.
     * @param approver_id Authenticated user approving the reservation.
     * @param approval_event_id Identifier for the approval domain event.
     * @param occurred_at Timestamp applied to the approval transition.
     */
    ApproveReservationCommand(
        haven::domain::OrganizationId organization_id,
        haven::domain::ReservationId reservation_id,
        haven::domain::UserId approver_id,
        haven::domain::EventId approval_event_id,
        TimePoint occurred_at)
        : organization_id_(std::move(organization_id)),
          reservation_id_(std::move(reservation_id)),
          approver_id_(std::move(approver_id)),
          approval_event_id_(std::move(approval_event_id)),
          occurred_at_(occurred_at) {}

    /**
     * @brief Returns the organization used to scope the operation.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the reservation requested for approval.
     */
    [[nodiscard]] const haven::domain::ReservationId& reservation_id() const noexcept {
        return reservation_id_;
    }

    /**
     * @brief Returns the user approving the reservation.
     */
    [[nodiscard]] const haven::domain::UserId& approver_id() const noexcept {
        return approver_id_;
    }

    /**
     * @brief Returns the approval event identifier.
     */
    [[nodiscard]] const haven::domain::EventId& approval_event_id() const noexcept {
        return approval_event_id_;
    }

    /**
     * @brief Returns the approval transition timestamp.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept {
        return occurred_at_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ReservationId reservation_id_;
    haven::domain::UserId approver_id_;
    haven::domain::EventId approval_event_id_;
    TimePoint occurred_at_;
};

}  // namespace haven::application::reservations