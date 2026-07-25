/**
 * @file reservation_cancelled_event.hpp
 * @brief Defines the reservation-cancelled domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records that a reservation was cancelled.
 *
 * Cancellation is valid for pending and confirmed reservations. The previous
 * status is retained so downstream consumers can distinguish whether a
 * confirmed schedule claim was released.
 */
class ReservationCancelledEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-cancelled event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which cancellation occurred.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Cancelled reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param cancelled_by User who cancelled the reservation.
     * @param previous_status Status immediately before cancellation.
     *
     * @throws std::invalid_argument when the previous status is neither
     * PendingApproval nor Confirmed.
     */
    ReservationCancelledEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        UserId cancelled_by,
        ReservationStatus previous_status);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which cancellation occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the cancelled reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the user who cancelled the reservation.
     */
    [[nodiscard]] const UserId& cancelled_by() const noexcept;

    /**
     * @brief Returns the status immediately before cancellation.
     */
    [[nodiscard]] ReservationStatus previous_status() const noexcept;

    auto operator<=>(const ReservationCancelledEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    UserId cancelled_by_;
    ReservationStatus previous_status_;
};

}  // namespace haven::domain