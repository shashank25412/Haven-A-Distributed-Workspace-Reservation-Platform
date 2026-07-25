/**
 * @file reservation_created_event.cpp
 * @brief Implements the reservation-created domain event.
 */

#include "haven/domain/events/reservation_created_event.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

ReservationCreatedEvent::ReservationCreatedEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    UserId created_by,
    TimeInterval interval,
    const ReservationKind kind,
    const ReservationStatus initial_status)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      created_by_(std::move(created_by)),
      interval_(std::move(interval)),
      kind_(kind),
      initial_status_(initial_status) {
    if (initial_status_ != ReservationStatus::PendingApproval && initial_status_ != ReservationStatus::Confirmed) {
        throw std::invalid_argument("Reservation-created event requires a valid initial reservation status.");
    }
}

const EventId& ReservationCreatedEvent::event_id() const noexcept {
    return event_id_;
}

ReservationCreatedEvent::TimePoint ReservationCreatedEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationCreatedEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationCreatedEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationCreatedEvent::resource_id() const noexcept {
    return resource_id_;
}

const UserId& ReservationCreatedEvent::created_by() const noexcept {
    return created_by_;
}

const TimeInterval& ReservationCreatedEvent::interval() const noexcept {
    return interval_;
}

ReservationKind ReservationCreatedEvent::kind() const noexcept {
    return kind_;
}

ReservationStatus ReservationCreatedEvent::initial_status() const noexcept {
    return initial_status_;
}

}  // namespace haven::domain