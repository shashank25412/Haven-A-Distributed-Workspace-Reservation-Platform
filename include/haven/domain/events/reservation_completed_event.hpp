/**
 * @file reservation_completed_event.hpp
 * @brief Defines the reservation-completed domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records that a confirmed reservation was completed.
 *
 * Completion is a system-driven lifecycle transition after the reserved
 * interval has finished. No user actor is recorded unless a future business
 * requirement introduces manual completion.
 */
class ReservationCompletedEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-completed event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which completion occurred.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Completed reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param interval Completed reservation interval.
     */
    ReservationCompletedEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        TimeInterval interval);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which completion occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the completed reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the completed reservation interval.
     */
    [[nodiscard]] const TimeInterval& interval() const noexcept;

    auto operator<=>(const ReservationCompletedEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    TimeInterval interval_;
};

}  // namespace haven::domain