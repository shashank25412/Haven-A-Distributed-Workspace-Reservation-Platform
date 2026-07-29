/**
 * @file couchbase_resource_repository.cpp
 * @brief Implements the Couchbase-backed resource repository adapter.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_resource_repository.hpp"

#include "haven/application/resources/resource_repository_error.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"
#include "haven/infrastructure/persistence/couchbase/resource_document_mapper.hpp"
#include "haven/logging/logging.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/query_options.hxx>
#include <couchbase/query_scan_consistency.hxx>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {

namespace {

using haven::application::resources::ResourceRepositoryError;
using haven::application::resources::ResourceRepositoryErrorCode;

[[nodiscard]] ResourceRepositoryError translate_error(const ::couchbase::error& error,
                                                      const std::string_view operation) {
    const auto error_code = error.ec();

    auto code = ResourceRepositoryErrorCode::Persistence;
    if (error_code == ::couchbase::errc::key_value::document_exists) {
        code = ResourceRepositoryErrorCode::AlreadyExists;
    } else if (error_code == ::couchbase::errc::common::authentication_failure) {
        code = ResourceRepositoryErrorCode::Authentication;
    } else if (error_code == ::couchbase::errc::key_value::xattr_no_access) {
        code = ResourceRepositoryErrorCode::Authorization;
    } else if (error_code == ::couchbase::errc::common::ambiguous_timeout ||
               error_code == ::couchbase::errc::common::unambiguous_timeout) {
        code = ResourceRepositoryErrorCode::Timeout;
    }

    return ResourceRepositoryError{code,
                                   std::string{operation} + " failed: " + error_code.message()};
}

[[nodiscard]] std::string active_resource_query() {
    return "SELECT resource.* FROM `" + std::string{CouchbaseCollections::resources} +
           "` AS resource WHERE resource.documentType = \"resource\" "
           "AND resource.organizationId = $organizationId "
           "AND resource.resourceType = $resourceType "
           "AND resource.status = \"ACTIVE\"";
}

}  // namespace

CouchbaseResourceRepository::CouchbaseResourceRepository(
    std::shared_ptr<CouchbaseConnection> connection)
    : connection_(std::move(connection)) {
    if (!connection_) {
        throw std::invalid_argument("Couchbase resource repository connection must not be null");
    }
}

haven::application::resources::ResourceLookupResult CouchbaseResourceRepository::find_by_id(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) const {
    HVN_TRACE_SCOPE();

    const auto document_key = resource_document_key(organization_id, resource_id);
    HVN_DEBUG_LOG("Reading Couchbase resource document with key {}", document_key);

    auto collection = connection_->collection(CouchbaseCollections::resources);
    auto [error, result] = collection.get(document_key).get();

    if (error.ec() == ::couchbase::errc::key_value::document_not_found) {
        return std::nullopt;
    }
    if (error) {
        HVN_ERROR_LOG("Couchbase resource read failed for organization {} and resource {}: {}",
                      organization_id.value(),
                      resource_id.value(),
                      error.ec().message());
        throw translate_error(error, "Couchbase resource read");
    }

    try {
        const auto json = result.content_as<tao::json::value>();
        auto resource = to_domain_resource(resource_document_from_json(json));
        if (resource.organization_id() != organization_id ||
            resource.resource_id() != resource_id) {
            throw std::invalid_argument("Stored resource identity does not match its document key");
        }
        return resource;
    } catch (const ResourceRepositoryError&) {
        throw;
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG(
            "Stored Couchbase resource is invalid for organization {} and resource {}: {}",
            organization_id.value(),
            resource_id.value(),
            exception.what());
        throw ResourceRepositoryError{ResourceRepositoryErrorCode::Persistence,
                                      "Stored Couchbase resource document is invalid"};
    }
}

haven::application::resources::ResourceSearchResult
CouchbaseResourceRepository::find_active_by_type(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceType resource_type) const {
    HVN_TRACE_SCOPE();

    auto options = ::couchbase::query_options{};
    options.readonly(true)
        .scan_consistency(::couchbase::query_scan_consistency::request_plus)
        .named_parameters(
            std::make_pair("organizationId", organization_id.value()),
            std::make_pair("resourceType", std::string{haven::domain::to_string(resource_type)}));

    auto [error, result] = connection_->scope().query(active_resource_query(), options).get();
    if (error) {
        HVN_ERROR_LOG("Couchbase resource search failed for organization {}: {}",
                      organization_id.value(),
                      error.ec().message());
        throw translate_error(error, "Couchbase resource search");
    }

    auto resources = haven::application::resources::ResourceSearchResult{};
    const auto rows = result.rows_as();
    resources.reserve(rows.size());
    try {
        for (const auto& row : rows) {
            auto resource = to_domain_resource(resource_document_from_json(row));
            if (resource.organization_id() != organization_id || resource.type() != resource_type ||
                !resource.is_active()) {
                throw std::invalid_argument(
                    "Resource search returned a document outside its filter");
            }
            resources.push_back(std::move(resource));
        }
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG(
            "Couchbase resource search returned an invalid document for organization {}: {}",
            organization_id.value(),
            exception.what());
        throw ResourceRepositoryError{ResourceRepositoryErrorCode::Persistence,
                                      "Couchbase resource search returned an invalid document"};
    }

    return resources;
}

}  // namespace haven::infrastructure::persistence::couchbase
