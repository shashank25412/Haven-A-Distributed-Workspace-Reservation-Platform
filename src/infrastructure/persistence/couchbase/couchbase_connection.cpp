/**
 * @file couchbase_connection.cpp
 * @brief Implements Couchbase connection ownership and collection access.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include "haven/logging/logging.hpp"

#include <couchbase/cluster_options.hxx>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {

namespace {

[[nodiscard]] ::couchbase::cluster connect_to_cluster(const CouchbaseConfiguration& configuration) {
    HVN_TRACE_SCOPE();

    auto options = ::couchbase::cluster_options(configuration.username, configuration.password);

    auto [error, cluster] =
        ::couchbase::cluster::connect(configuration.connection_string, options).get();

    if (error) {
        throw std::runtime_error("Failed to connect to Couchbase: " + error.ec().message());
    }

    return cluster;
}

}  // namespace

CouchbaseConnection::CouchbaseConnection(CouchbaseConfiguration configuration)
    : configuration_(std::move(configuration)), cluster_([this] {
          validate_configuration(configuration_);
          return connect_to_cluster(configuration_);
      }()) {
    HVN_INFO_LOG("Connected to Couchbase cluster");
}

CouchbaseConnection::~CouchbaseConnection() {
    HVN_TRACE_SCOPE();

    try {
        cluster_.close().get();
        HVN_INFO_LOG("Closed Couchbase cluster connection");
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG("Failed to close Couchbase cluster connection: ", exception.what());
    }
}

::couchbase::collection CouchbaseConnection::collection(const std::string_view collection_name) {
    HVN_TRACE_SCOPE();

    if (collection_name.empty()) {
        throw std::invalid_argument("Couchbase collection name must not be empty");
    }

    return cluster_.bucket(configuration_.bucket_name)
        .scope(configuration_.scope_name)
        .collection(std::string{collection_name});
}

::couchbase::scope CouchbaseConnection::scope() {
    HVN_TRACE_SCOPE();

    return cluster_.bucket(configuration_.bucket_name).scope(configuration_.scope_name);
}

std::shared_ptr<::couchbase::transactions::transactions> CouchbaseConnection::transactions() const {
    return cluster_.transactions();
}

bool CouchbaseConnection::is_ready() const noexcept {
    try {
        const auto [error, result] = cluster_.ping().get();
        static_cast<void>(result);
        return !error;
    } catch (...) {
        return false;
    }
}

void CouchbaseConnection::validate_configuration(const CouchbaseConfiguration& configuration) {
    HVN_TRACE_SCOPE();

    if (configuration.connection_string.empty()) {
        throw std::invalid_argument("Couchbase connection string must not be empty");
    }

    if (configuration.username.empty()) {
        throw std::invalid_argument("Couchbase username must not be empty");
    }

    if (configuration.password.empty()) {
        throw std::invalid_argument("Couchbase password must not be empty");
    }

    if (configuration.bucket_name.empty()) {
        throw std::invalid_argument("Couchbase bucket name must not be empty");
    }

    if (configuration.scope_name.empty()) {
        throw std::invalid_argument("Couchbase scope name must not be empty");
    }
}

}  // namespace haven::infrastructure::persistence::couchbase
