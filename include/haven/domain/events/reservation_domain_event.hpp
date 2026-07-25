/**
 * @file reservation_domain_event.hpp
 * @brief Defines the supported reservation domain-event variant.
 */

#pragma once

#include "haven/domain/events/reservation_approval_requested_event.hpp"
#include "haven/domain/events/reservation_cancelled_event.hpp"
#include "haven/domain/events/reservation_completed_event.hpp"
#include "haven/domain/events/reservation_confirmed_event.hpp"
#include "haven/domain/events/reservation_created_event.hpp"
#include "haven/domain/events/reservation_expired_event.hpp"
#include "haven/domain/events/reservation_extended_event.hpp"
#include "haven/domain/events/reservation_rejected_event.hpp"

#include <variant>

namespace haven::domain {

/**
 * @brief Represents a business event emitted by the reservation aggregate.
 *
 * Infrastructure adapters may later translate these typed events into outbox
 * documents and Kafka event envelopes.
 */
using ReservationDomainEvent = std::variant<
    ReservationCreatedEvent,
    ReservationApprovalRequestedEvent,
    ReservationConfirmedEvent,
    ReservationRejectedEvent,
    ReservationCancelledEvent,
    ReservationExtendedEvent,
    ReservationExpiredEvent,
    ReservationCompletedEvent>;

}  // namespace haven::domain