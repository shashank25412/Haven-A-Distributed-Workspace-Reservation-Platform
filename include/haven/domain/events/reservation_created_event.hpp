/**
 * @file reservation_created_event.hpp
 * @brief Defines the reservation-created domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records the successful creation of a reservation.
 *
 * A newly created reservation may be confirmed immediately or may await
 * approval. Terminal reservation states are not valid initial states.
 *
 * Free-form purpose text is intentionally excluded to minimize unnecessary
 * information in downstream events.
 */
class ReservationCreatedEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-created event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which the reservation was created.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Created reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param created_by User who created the reservation.
     * @param interval Requested reservation interval.
     * @param kind Reservation kind.
     * @param initial_status Initial reservation status.
     *
     * @throws std::invalid_argument when the initial status is neither
     * PendingApproval nor Confirmed.
     */
    ReservationCreatedEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        UserId created_by,
        TimeInterval interval,
        ReservationKind kind,
        ReservationStatus initial_status);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which the event occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the created reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the user who created the reservation.
     */
    [[nodiscard]] const UserId& created_by() const noexcept;

    /**
     * @brief Returns the requested reservation interval.
     */
    [[nodiscard]] const TimeInterval& interval() const noexcept;

    /**
     * @brief Returns the reservation kind.
     */
    [[nodiscard]] ReservationKind kind() const noexcept;

    /**
     * @brief Returns the reservation status immediately after creation.
     */
    [[nodiscard]] ReservationStatus initial_status() const noexcept;

    auto operator<=>(const ReservationCreatedEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    UserId created_by_;
    TimeInterval interval_;
    ReservationKind kind_;
    ReservationStatus initial_status_;
};

}  // namespace haven::domain