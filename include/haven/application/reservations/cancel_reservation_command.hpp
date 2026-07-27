/**
 * @file cancel_reservation_command.hpp
 * @brief Defines the input command for cancelling a reservation.
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
 * @brief Describes a caller's request to cancel one reservation.
 *
 * Event identifier generation and clock access remain outside the handler,
 * keeping the use case deterministic and independently testable.
 */
class CancelReservationCommand final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation cancellation command.
     *
     * @param organization_id Organization visible to the caller.
     * @param reservation_id Reservation requested for cancellation.
     * @param caller_id Authenticated caller performing the operation.
     * @param cancellation_event_id Identifier for the cancellation domain event.
     * @param occurred_at Timestamp applied to the cancellation event.
     */
    CancelReservationCommand(
        haven::domain::OrganizationId organization_id,
        haven::domain::ReservationId reservation_id,
        haven::domain::UserId caller_id,
        haven::domain::EventId cancellation_event_id,
        TimePoint occurred_at)
        : organization_id_(std::move(organization_id)),
          reservation_id_(std::move(reservation_id)),
          caller_id_(std::move(caller_id)),
          cancellation_event_id_(std::move(cancellation_event_id)),
          occurred_at_(occurred_at) {}

    /**
     * @brief Returns the organization used to scope the operation.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the reservation requested for cancellation.
     */
    [[nodiscard]] const haven::domain::ReservationId& reservation_id() const noexcept {
        return reservation_id_;
    }

    /**
     * @brief Returns the authenticated caller.
     */
    [[nodiscard]] const haven::domain::UserId& caller_id() const noexcept {
        return caller_id_;
    }

    /**
     * @brief Returns the cancellation event identifier.
     */
    [[nodiscard]] const haven::domain::EventId& cancellation_event_id() const noexcept {
        return cancellation_event_id_;
    }

    /**
     * @brief Returns the cancellation event timestamp.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept {
        return occurred_at_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ReservationId reservation_id_;
    haven::domain::UserId caller_id_;
    haven::domain::EventId cancellation_event_id_;
    TimePoint occurred_at_;
};

}  // namespace haven::application::reservations