/**
 * @file reservation_creation_store.hpp
 * @brief Defines atomic persistence for a newly created reservation and its events.
 */

#pragma once

#include "haven/application/persistence/persistence_token.hpp"
#include "haven/domain/events/reservation_domain_event.hpp"
#include "haven/domain/reservation.hpp"
#include "haven/domain/value_objects/organization_id.hpp"

#include <vector>

namespace haven::application::reservations {

/**
 * @brief Persists a reservation creation and all events produced by that creation.
 *
 * On success, the reservation and every supplied event are durably persisted as
 * one logical unit. On failure, an implementation must not report success for a
 * partially persisted unit.
 *
 * This application-owned contract is persistence-technology neutral. Adapters
 * are responsible for providing the required all-or-nothing behavior.
 */
class ReservationCreationStore {
public:
    virtual ~ReservationCreationStore() = default;

    /**
     * @brief Atomically persists a newly created reservation and its ordered events.
     *
     * @param organization_id Organization that owns the reservation.
     * @param reservation Newly created reservation aggregate.
     * @param domain_events Events produced during creation, in occurrence order.
     * @return Opaque persistence token for the persisted reservation revision.
     */
    [[nodiscard]] virtual persistence::PersistenceToken persist(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        std::vector<haven::domain::ReservationDomainEvent> domain_events) = 0;
};

}  // namespace haven::application::reservations
