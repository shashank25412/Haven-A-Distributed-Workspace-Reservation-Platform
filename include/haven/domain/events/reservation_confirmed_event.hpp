/**
 * @file reservation_confirmed_event.hpp
 * @brief Defines the reservation-confirmed domain event.
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
#include <optional>

namespace haven::domain {

/**
 * @brief Records that a reservation transitioned to the confirmed state.
 *
 * The confirming user is absent for automatically confirmed reservations and
 * present when a pending reservation is confirmed through approval.
 */
class ReservationConfirmedEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-confirmed event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which confirmation occurred.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Confirmed reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param interval Confirmed reservation interval.
     * @param confirmed_by User who approved the reservation, when applicable.
     */
    ReservationConfirmedEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        TimeInterval interval,
        std::optional<UserId> confirmed_by);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which confirmation occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the confirmed reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the confirmed reservation interval.
     */
    [[nodiscard]] const TimeInterval& interval() const noexcept;

    /**
     * @brief Returns the confirming user when confirmation followed approval.
     */
    [[nodiscard]] const std::optional<UserId>& confirmed_by() const noexcept;

    auto operator<=>(const ReservationConfirmedEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    TimeInterval interval_;
    std::optional<UserId> confirmed_by_;
};

}  // namespace haven::domain