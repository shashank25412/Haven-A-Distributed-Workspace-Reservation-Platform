/**
 * @file reservation_expired_event.hpp
 * @brief Defines the reservation-expired domain event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Records that a reservation expired.
 *
 * The previous status distinguishes an expired pending approval from an
 * expired confirmed reservation whose schedule claim must be released.
 */
class ReservationExpiredEvent final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation-expired event.
     *
     * @param event_id Unique identifier for this event.
     * @param occurred_at Time at which expiration occurred.
     * @param organization_id Organization that owns the reservation.
     * @param reservation_id Expired reservation identifier.
     * @param resource_id Reserved resource identifier.
     * @param previous_status Status immediately before expiration.
     *
     * @throws std::invalid_argument when the previous status is neither
     * PendingApproval nor Confirmed.
     */
    ReservationExpiredEvent(
        EventId event_id,
        TimePoint occurred_at,
        OrganizationId organization_id,
        ReservationId reservation_id,
        ResourceId resource_id,
        ReservationStatus previous_status);

    /**
     * @brief Returns the unique event identifier.
     */
    [[nodiscard]] const EventId& event_id() const noexcept;

    /**
     * @brief Returns the time at which expiration occurred.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept;

    /**
     * @brief Returns the organization that owns the reservation.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the expired reservation identifier.
     */
    [[nodiscard]] const ReservationId& reservation_id() const noexcept;

    /**
     * @brief Returns the reserved resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the status immediately before expiration.
     */
    [[nodiscard]] ReservationStatus previous_status() const noexcept;

    auto operator<=>(const ReservationExpiredEvent&) const = default;

private:
    EventId event_id_;
    TimePoint occurred_at_;
    OrganizationId organization_id_;
    ReservationId reservation_id_;
    ResourceId resource_id_;
    ReservationStatus previous_status_;
};

}  // namespace haven::domain