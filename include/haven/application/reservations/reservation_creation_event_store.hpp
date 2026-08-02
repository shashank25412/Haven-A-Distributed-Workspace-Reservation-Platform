/**
 * @file reservation_creation_event_store.hpp
 * @brief Defines durable creation-event verification for recovery.
 */
#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"

#include <vector>

namespace haven::application::reservations {

/** @brief Verifies that expected Reservation creation events are durably persisted. */
class ReservationCreationEventStore {
public:
    virtual ~ReservationCreationEventStore() = default;
    [[nodiscard]] virtual bool contains_all(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id,
        const std::vector<haven::domain::EventId>& event_ids) const = 0;
};

}  // namespace haven::application::reservations
