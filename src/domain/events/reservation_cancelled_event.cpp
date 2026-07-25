/**
 * @file reservation_cancelled_event.cpp
 * @brief Implements the reservation-cancelled domain event.
 */

#include "haven/domain/events/reservation_cancelled_event.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

ReservationCancelledEvent::ReservationCancelledEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    UserId cancelled_by,
    const ReservationStatus previous_status)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      cancelled_by_(std::move(cancelled_by)),
      previous_status_(previous_status) {
    if (previous_status_ != ReservationStatus::PendingApproval && previous_status_ != ReservationStatus::Confirmed) {
        throw std::invalid_argument("Reservation-cancelled event requires a cancellable previous status.");
    }
}

const EventId& ReservationCancelledEvent::event_id() const noexcept {
    return event_id_;
}

ReservationCancelledEvent::TimePoint ReservationCancelledEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationCancelledEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationCancelledEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationCancelledEvent::resource_id() const noexcept {
    return resource_id_;
}

const UserId& ReservationCancelledEvent::cancelled_by() const noexcept {
    return cancelled_by_;
}

ReservationStatus ReservationCancelledEvent::previous_status() const noexcept {
    return previous_status_;
}

}  // namespace haven::domain