/**
 * @file reservation_rejected_event.cpp
 * @brief Implements the reservation-rejected domain event.
 */

#include "haven/domain/events/reservation_rejected_event.hpp"

#include <utility>

namespace haven::domain {

ReservationRejectedEvent::ReservationRejectedEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    UserId rejected_by)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      rejected_by_(std::move(rejected_by)) {
}

const EventId& ReservationRejectedEvent::event_id() const noexcept {
    return event_id_;
}

ReservationRejectedEvent::TimePoint ReservationRejectedEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationRejectedEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationRejectedEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationRejectedEvent::resource_id() const noexcept {
    return resource_id_;
}

const UserId& ReservationRejectedEvent::rejected_by() const noexcept {
    return rejected_by_;
}

}  // namespace haven::domain