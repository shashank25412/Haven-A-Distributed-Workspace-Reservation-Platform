/**
 * @file couchbase_connection.hpp
 * @brief Declares the shared Couchbase connection owner.
 */

#pragma once

#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"

#include <couchbase/cluster.hxx>
#include <couchbase/collection.hxx>
#include <couchbase/scope.hxx>
#include <memory>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Owns the Couchbase cluster connection used by persistence adapters.
 *
 * A single instance is intended to be created by the application composition
 * root and shared by Couchbase-backed repositories. The object owns the SDK
 * cluster lifecycle and provides access to collections within the configured
 * bucket and scope.
 */
class CouchbaseConnection {
public:
    /**
     * @brief Connects to Couchbase using the supplied configuration.
     *
     * @param configuration Couchbase endpoint, credentials, bucket, and scope.
     *
     * @throws std::invalid_argument If required configuration is missing.
     * @throws std::runtime_error If the Couchbase connection fails.
     */
    explicit CouchbaseConnection(CouchbaseConfiguration configuration);

    /**
     * @brief Closes the owned Couchbase cluster connection.
     */
    ~CouchbaseConnection();

    CouchbaseConnection(const CouchbaseConnection&) = delete;
    CouchbaseConnection& operator=(const CouchbaseConnection&) = delete;
    CouchbaseConnection(CouchbaseConnection&&) = delete;
    CouchbaseConnection& operator=(CouchbaseConnection&&) = delete;

    /**
     * @brief Returns a collection from the configured bucket and scope.
     *
     * @param collection_name Name of the requested collection.
     *
     * @return Couchbase SDK collection handle.
     *
     * @throws std::invalid_argument If the collection name is empty.
     */
    [[nodiscard]] ::couchbase::collection collection(std::string_view collection_name);

    /**
     * @brief Returns the configured Couchbase scope.
     *
     * @return Couchbase SDK scope handle used by infrastructure queries.
     */
    [[nodiscard]] ::couchbase::scope scope();

    /** @brief Returns the transaction manager for the owned cluster connection. */
    [[nodiscard]] std::shared_ptr<::couchbase::transactions::transactions> transactions() const;

private:
    static void validate_configuration(const CouchbaseConfiguration& configuration);

    CouchbaseConfiguration configuration_;
    ::couchbase::cluster cluster_;
};

}  // namespace haven::infrastructure::persistence::couchbase
