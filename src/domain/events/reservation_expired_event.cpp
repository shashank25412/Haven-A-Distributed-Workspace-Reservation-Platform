/**
 * @file reservation_expired_event.cpp
 * @brief Implements the reservation-expired domain event.
 */

#include "haven/domain/events/reservation_expired_event.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

ReservationExpiredEvent::ReservationExpiredEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    const ReservationStatus previous_status)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      previous_status_(previous_status) {
    if (previous_status_ != ReservationStatus::PendingApproval && previous_status_ != ReservationStatus::Confirmed) {
        throw std::invalid_argument("Reservation-expired event requires an expirable previous status.");
    }
}

const EventId& ReservationExpiredEvent::event_id() const noexcept {
    return event_id_;
}

ReservationExpiredEvent::TimePoint ReservationExpiredEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationExpiredEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationExpiredEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationExpiredEvent::resource_id() const noexcept {
    return resource_id_;
}

ReservationStatus ReservationExpiredEvent::previous_status() const noexcept {
    return previous_status_;
}

}  // namespace haven::domain