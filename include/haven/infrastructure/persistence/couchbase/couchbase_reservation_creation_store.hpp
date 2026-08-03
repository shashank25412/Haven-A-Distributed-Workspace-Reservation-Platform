/** @file couchbase_reservation_creation_store.hpp */
#pragma once

#include "haven/application/reservations/reservation_creation_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/metrics/couchbase_persistence_metrics.hpp"

#include <memory>

namespace haven::infrastructure::persistence::couchbase {

/** @brief Atomically persists a new Reservation and all of its Outbox events. */
class CouchbaseReservationCreationStore final
    : public haven::application::reservations::ReservationCreationStore {
public:
    CouchbaseReservationCreationStore(
        std::shared_ptr<CouchbaseConnection> connection,
        application::observability::metrics::MetricsRecorder& recorder);

    [[nodiscard]] haven::application::persistence::PersistenceToken persist(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        std::vector<haven::domain::ReservationDomainEvent> domain_events) override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
    metrics::OperationMetrics metrics_;
};

}  // namespace haven::infrastructure::persistence::couchbase
