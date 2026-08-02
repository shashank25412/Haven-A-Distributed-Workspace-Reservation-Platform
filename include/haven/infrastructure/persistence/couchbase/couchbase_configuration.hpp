/**
 * @file couchbase_configuration.hpp
 * @brief Defines the configuration required for Couchbase persistence.
 */

#pragma once

#include <chrono>
#include <string>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Contains Couchbase connection and namespace configuration.
 *
 * The bootstrap layer is responsible for constructing this value from
 * environment configuration. Infrastructure components consume this value
 * without reading process environment variables directly.
 */
struct CouchbaseConfiguration {
    std::string connection_string;
    std::string username;
    std::string password;
    std::string bucket_name;
    std::string scope_name;
    std::chrono::seconds idempotency_retention{std::chrono::hours{24}};
};

}  // namespace haven::infrastructure::persistence::couchbase
