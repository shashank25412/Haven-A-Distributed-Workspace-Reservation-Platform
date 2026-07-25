/**
 * @file reservation_completed_event.cpp
 * @brief Implements the reservation-completed domain event.
 */

#include "haven/domain/events/reservation_completed_event.hpp"

#include <utility>

namespace haven::domain {

ReservationCompletedEvent::ReservationCompletedEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    TimeInterval interval)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      interval_(std::move(interval)) {
}

const EventId& ReservationCompletedEvent::event_id() const noexcept {
    return event_id_;
}

ReservationCompletedEvent::TimePoint ReservationCompletedEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationCompletedEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationCompletedEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationCompletedEvent::resource_id() const noexcept {
    return resource_id_;
}

const TimeInterval& ReservationCompletedEvent::interval() const noexcept {
    return interval_;
}

}  // namespace haven::domain