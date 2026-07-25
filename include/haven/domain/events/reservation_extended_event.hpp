/**
 * @file reservation_extended_event.hpp
 * @brief Defines the reservation-extended domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records that a confirmed reservation interval was extended.
 *
 * Both the previous and extended intervals are retained so downstream
 * consumers can determine how the confirmed resource schedule changed.
 */
class ReservationExtendedEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-extended event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which the extension occurred.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Extended reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param extended_by User who extended the reservation.
     * @param previous_interval Interval immediately before the extension.
     * @param extended_interval Interval immediately after the extension.
     *
     * @throws std::invalid_argument when the interval start changes or the
     * extended end time does not move forward.
     */
    ReservationExtendedEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        UserId extended_by,
        TimeInterval previous_interval,
        TimeInterval extended_interval);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which the extension occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the extended reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the user who extended the reservation.
     */
    [[nodiscard]] const UserId& extended_by() const noexcept;

    /**
     * @brief Returns the interval immediately before the extension.
     */
    [[nodiscard]] const TimeInterval& previous_interval() const noexcept;

    /**
     * @brief Returns the interval immediately after the extension.
     */
    [[nodiscard]] const TimeInterval& extended_interval() const noexcept;

    auto operator<=>(const ReservationExtendedEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    UserId extended_by_;
    TimeInterval previous_interval_;
    TimeInterval extended_interval_;
};

}  // namespace haven::domain