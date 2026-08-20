/**
 * @file couchbase_reservation_repository.hpp
 * @brief Declares the Couchbase-backed reservation repository adapter.
 */

#pragma once

#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/metrics/couchbase_persistence_metrics.hpp"

#include <memory>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Implements the application reservation repository port with Couchbase.
 *
 * Point reads use tenant-scoped document keys and queries include an explicit
 * organization predicate. Couchbase types remain inside infrastructure, and
 * persisted state is restored through ReservationDocumentMapper.
 */
class CouchbaseReservationRepository final
    : public haven::application::reservations::ReservationRepository {
public:
    /**
     * @brief Constructs a repository using a shared Couchbase connection.
     *
     * @throws std::invalid_argument If connection is null.
     */
    CouchbaseReservationRepository(std::shared_ptr<CouchbaseConnection> connection,
                                   application::observability::metrics::MetricsRecorder& recorder);

    [[nodiscard]] haven::application::reservations::ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id) const override;

    [[nodiscard]] haven::application::reservations::ReservationListResult find_by_creator(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::UserId& caller_id) const override;

    [[nodiscard]] haven::application::reservations::ReservationListResult find_pending_approvals(
        const haven::domain::OrganizationId& organization_id) const override;

    [[nodiscard]] haven::application::reservations::ReservationListResult find_decided_approvals(
        const haven::domain::OrganizationId& organization_id) const override;

    [[nodiscard]] haven::application::reservations::ReservationListResult find_all(
        const haven::domain::OrganizationId& organization_id) const override;

    /**
     * @brief Returns reservations overlapping an interval using `[start, end)` semantics.
     */
    [[nodiscard]] haven::application::reservations::ReservationListResult
    find_by_resource_and_interval(const haven::domain::OrganizationId& organization_id,
                                  const haven::domain::ResourceId& resource_id,
                                  const haven::domain::TimeInterval& interval) const override;

    [[nodiscard]] bool has_conflict(const haven::domain::OrganizationId& organization_id,
                                    const haven::domain::ResourceId& resource_id,
                                    const haven::domain::TimeInterval& interval) const override;

    [[nodiscard]] bool has_conflict_excluding(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::TimeInterval& interval,
        const haven::domain::ReservationId& excluded_reservation_id) const override;

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation) override;

    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        const haven::application::persistence::PersistenceToken& expected_token) override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
    metrics::OperationMetrics metrics_;
};

}  // namespace haven::infrastructure::persistence::couchbase
