/**
 * @file reservation_extended_event.cpp
 * @brief Implements the reservation-extended domain event.
 */

#include "haven/domain/events/reservation_extended_event.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

ReservationExtendedEvent::ReservationExtendedEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    UserId extended_by,
    TimeInterval previous_interval,
    TimeInterval extended_interval)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      extended_by_(std::move(extended_by)),
      previous_interval_(std::move(previous_interval)),
      extended_interval_(std::move(extended_interval)) {
    if (previous_interval_.start() != extended_interval_.start()) {
        throw std::invalid_argument("Reservation extension must preserve the original start time.");
    }

    if (extended_interval_.end() <= previous_interval_.end()) {
        throw std::invalid_argument("Reservation extension must move the end time forward.");
    }
}

const EventId& ReservationExtendedEvent::event_id() const noexcept {
    return event_id_;
}

ReservationExtendedEvent::TimePoint ReservationExtendedEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationExtendedEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationExtendedEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationExtendedEvent::resource_id() const noexcept {
    return resource_id_;
}

const UserId& ReservationExtendedEvent::extended_by() const noexcept {
    return extended_by_;
}

const TimeInterval& ReservationExtendedEvent::previous_interval() const noexcept {
    return previous_interval_;
}

const TimeInterval& ReservationExtendedEvent::extended_interval() const noexcept {
    return extended_interval_;
}

}  // namespace haven::domain