/**
 * @file couchbase_reservation_creation_event_store.hpp
 * @brief Declares Couchbase creation-event recovery reads.
 */
#pragma once

#include "haven/application/reservations/reservation_creation_event_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/metrics/couchbase_persistence_metrics.hpp"

#include <memory>

namespace haven::infrastructure::persistence::couchbase {

class CouchbaseReservationCreationEventStore final
    : public haven::application::reservations::ReservationCreationEventStore {
public:
    explicit CouchbaseReservationCreationEventStore(
        std::shared_ptr<CouchbaseConnection> connection,
        application::observability::metrics::MetricsRecorder& recorder);
    [[nodiscard]] bool contains_all(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id,
        const std::vector<haven::domain::EventId>& event_ids) const override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
    metrics::OperationMetrics metrics_;
};

}  // namespace haven::infrastructure::persistence::couchbase
