/**
 * @file reservation_approval_requested_event.cpp
 * @brief Implements the reservation-approval-requested domain event.
 */

#include "haven/domain/events/reservation_approval_requested_event.hpp"

#include <utility>

namespace haven::domain {

ReservationApprovalRequestedEvent::ReservationApprovalRequestedEvent(
    EventId event_id,
    const TimePoint occurred_at,
    OrganizationId organization_id,
    ReservationId reservation_id,
    ResourceId resource_id,
    UserId requested_by,
    TimeInterval interval,
    const ReservationKind kind)
    : event_id_(std::move(event_id)),
      occurred_at_(occurred_at),
      organization_id_(std::move(organization_id)),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      requested_by_(std::move(requested_by)),
      interval_(std::move(interval)),
      kind_(kind) {
}

const EventId& ReservationApprovalRequestedEvent::event_id() const noexcept {
    return event_id_;
}

ReservationApprovalRequestedEvent::TimePoint ReservationApprovalRequestedEvent::occurred_at() const noexcept {
    return occurred_at_;
}

const OrganizationId& ReservationApprovalRequestedEvent::organization_id() const noexcept {
    return organization_id_;
}

const ReservationId& ReservationApprovalRequestedEvent::reservation_id() const noexcept {
    return reservation_id_;
}

const ResourceId& ReservationApprovalRequestedEvent::resource_id() const noexcept {
    return resource_id_;
}

const UserId& ReservationApprovalRequestedEvent::requested_by() const noexcept {
    return requested_by_;
}

const TimeInterval& ReservationApprovalRequestedEvent::interval() const noexcept {
    return interval_;
}

ReservationKind ReservationApprovalRequestedEvent::kind() const noexcept {
    return kind_;
}

}  // namespace haven::domain