/**
 * @file reject_reservation_command.hpp
 * @brief Defines the input command for rejecting a reservation.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a request to reject a pending reservation.
 *
 * The rejecting user, event identifier, and timestamp are supplied by the
 * caller so the handler remains deterministic and independently testable.
 */
class RejectReservationCommand final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation rejection command.
     *
     * @param organization_id Organization used to scope the operation.
     * @param reservation_id Reservation requested for rejection.
     * @param rejected_by User rejecting the reservation.
     * @param rejection_event_id Identifier for the rejection domain event.
     * @param occurred_at Timestamp applied to the rejection transition.
     * @param reason Optional free-form explanation for the rejection.
     */
    RejectReservationCommand(
        haven::domain::OrganizationId organization_id,
        haven::domain::ReservationId reservation_id,
        haven::domain::UserId rejected_by,
        haven::domain::EventId rejection_event_id,
        TimePoint occurred_at,
        std::optional<std::string> reason = std::nullopt)
        : organization_id_(std::move(organization_id)),
          reservation_id_(std::move(reservation_id)),
          rejected_by_(std::move(rejected_by)),
          rejection_event_id_(std::move(rejection_event_id)),
          occurred_at_(occurred_at),
          reason_(std::move(reason)) {}

    /**
     * @brief Returns the organization used to scope the operation.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the reservation requested for rejection.
     */
    [[nodiscard]] const haven::domain::ReservationId& reservation_id() const noexcept {
        return reservation_id_;
    }

    /**
     * @brief Returns the user rejecting the reservation.
     */
    [[nodiscard]] const haven::domain::UserId& rejected_by() const noexcept {
        return rejected_by_;
    }

    /**
     * @brief Returns the rejection event identifier.
     */
    [[nodiscard]] const haven::domain::EventId& rejection_event_id() const noexcept {
        return rejection_event_id_;
    }

    /**
     * @brief Returns the rejection transition timestamp.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept {
        return occurred_at_;
    }

    /**
     * @brief Returns the optional rejection reason.
     */
    [[nodiscard]] const std::optional<std::string>& reason() const noexcept {
        return reason_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ReservationId reservation_id_;
    haven::domain::UserId rejected_by_;
    haven::domain::EventId rejection_event_id_;
    TimePoint occurred_at_;
    std::optional<std::string> reason_;
};

}  // namespace haven::application::reservations