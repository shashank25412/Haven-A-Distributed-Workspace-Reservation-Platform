/**
 * @file couchbase_organization_repository.hpp
 * @brief Declares the Couchbase-backed organization directory repository adapter.
 */

#pragma once

#include "haven/application/organizations/organization_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include <memory>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Implements the application organization repository port with Couchbase.
 */
class CouchbaseOrganizationRepository final
    : public haven::application::organizations::OrganizationRepository {
public:
    /**
     * @brief Constructs a repository using a shared Couchbase connection.
     *
     * @throws std::invalid_argument If connection is null.
     */
    explicit CouchbaseOrganizationRepository(std::shared_ptr<CouchbaseConnection> connection);

    [[nodiscard]] std::vector<haven::domain::Organization> find_all() const override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
};

}  // namespace haven::infrastructure::persistence::couchbase
