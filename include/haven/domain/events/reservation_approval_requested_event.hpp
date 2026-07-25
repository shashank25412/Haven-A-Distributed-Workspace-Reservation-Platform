/**
 * @file reservation_approval_requested_event.hpp
 * @brief Defines the reservation-approval-requested domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records that a reservation requires an approval decision.
 *
 * This event is emitted only for reservations created in the
 * PendingApproval state. It does not claim the confirmed resource schedule.
 *
 * Free-form purpose text is intentionally excluded from the event.
 */
class ReservationApprovalRequestedEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-approval-requested event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which approval was requested.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Reservation awaiting approval.
     * @param resource_id Requested resource identifier.
     * @param requested_by User who created the reservation.
     * @param interval Requested reservation interval.
     * @param kind Reservation kind.
     */
    ReservationApprovalRequestedEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        UserId requested_by,
        TimeInterval interval,
        ReservationKind kind);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which approval was requested.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the reservation awaiting approval.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the requested resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the user who requested the reservation.
     */
    [[nodiscard]] const UserId& requested_by() const noexcept;

    /**
     * @brief Returns the requested reservation interval.
     */
    [[nodiscard]] const TimeInterval& interval() const noexcept;

    /**
     * @brief Returns the reservation kind.
     */
    [[nodiscard]] ReservationKind kind() const noexcept;

    auto operator<=>(const ReservationApprovalRequestedEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    UserId requested_by_;
    TimeInterval interval_;
    ReservationKind kind_;
};

}  // namespace haven::domain