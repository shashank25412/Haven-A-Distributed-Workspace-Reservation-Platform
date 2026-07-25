/**
 * @file reservation_confirmed_event.cpp
 * @brief Implements the reservation-confirmed domain event.
 */

#include "haven/domain/events/reservation_confirmed_event.hpp"

#include <utility>

namespace haven::domain {

ReservationConfirmedEvent::ReservationConfirmedEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    TimeInterval interval,
    std::optional<UserId> confirmed_by)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      interval_(std::move(interval)),
      confirmed_by_(std::move(confirmed_by)) {
}

const EventId& ReservationConfirmedEvent::event_id() const noexcept {
    return event_id_;
}

ReservationConfirmedEvent::TimePoint ReservationConfirmedEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationConfirmedEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationConfirmedEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationConfirmedEvent::resource_id() const noexcept {
    return resource_id_;
}

const TimeInterval& ReservationConfirmedEvent::interval() const noexcept {
    return interval_;
}

const std::optional<UserId>& ReservationConfirmedEvent::confirmed_by() const noexcept {
    return confirmed_by_;
}

}  // namespace haven::domain