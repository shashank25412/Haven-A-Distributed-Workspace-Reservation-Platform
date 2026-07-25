/**
 * @file reservation_rejected_event.hpp
 * @brief Defines the reservation-rejected domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records that a pending reservation was rejected.
 *
 * Rejection occurs only through an explicit approval decision. Free-form
 * rejection reasons are intentionally excluded until a concrete business
 * requirement defines their validation and retention rules.
 */
class ReservationRejectedEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-rejected event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which rejection occurred.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Rejected reservation identifier.
     * @param resource_id Requested resource identifier.
     * @param rejected_by User who rejected the reservation.
     */
    ReservationRejectedEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        UserId rejected_by);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which rejection occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the rejected reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the requested resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the user who rejected the reservation.
     */
    [[nodiscard]] const UserId& rejected_by() const noexcept;

    auto operator<=>(const ReservationRejectedEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    UserId rejected_by_;
};

}  // namespace haven::domain