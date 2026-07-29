/**
 * @file couchbase_resource_repository.hpp
 * @brief Declares the Couchbase-backed resource repository adapter.
 */

#pragma once

#include "haven/application/resources/resource_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include <memory>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Implements the application resource repository port with Couchbase.
 *
 * Point reads enforce tenant isolation through tenant-scoped document keys.
 * Couchbase SDK and document representation concerns remain confined to the
 * infrastructure layer.
 */
class CouchbaseResourceRepository final : public haven::application::resources::ResourceRepository {
public:
    /**
     * @brief Constructs a repository using a shared Couchbase connection.
     *
     * @throws std::invalid_argument If connection is null.
     */
    explicit CouchbaseResourceRepository(std::shared_ptr<CouchbaseConnection> connection);

    [[nodiscard]] haven::application::resources::ResourceLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override;

    [[nodiscard]] haven::application::resources::ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId& organization_id,
        haven::domain::ResourceType resource_type) const override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
};

}  // namespace haven::infrastructure::persistence::couchbase
