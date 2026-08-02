/**
 * @file outbox_document_mapper.hpp
 * @brief Declares Reservation domain-event to Outbox document mapping.
 */

#pragma once

#include "haven/domain/events/reservation_domain_event.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

[[nodiscard]] OutboxDocument to_outbox_document(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::ReservationDomainEvent& event);

}  // namespace haven::infrastructure::persistence::couchbase
