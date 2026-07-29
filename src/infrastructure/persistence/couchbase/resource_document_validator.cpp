/**
 * @file resource_document_validator.cpp
 * @brief Implements validation for persisted Couchbase resource documents.
 */

#include "haven/infrastructure/persistence/couchbase/resource_document_validator.hpp"

#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

namespace {

void require_non_empty(
    const std::string_view value,
    const std::string_view field_name) {
    if (value.empty()) {
        throw std::invalid_argument(
            "Couchbase resource document field must not be empty: " +
            std::string{field_name});
    }
}

}  // namespace

void validate_resource_document(const ResourceDocument& document) {
    HVN_TRACE_SCOPE();

    if (document.schema_version != kResourceDocumentSchemaVersion) {
        throw std::invalid_argument(
            "Unsupported Couchbase resource document schema version: " +
            std::to_string(document.schema_version));
    }

    require_non_empty(document.resource_id, "resourceId");
    require_non_empty(document.organization_id, "organizationId");
    require_non_empty(document.name, "name");
    require_non_empty(document.resource_type, "resourceType");
    require_non_empty(document.status, "status");

    if (document.version == 0) {
        throw std::invalid_argument(
            "Couchbase resource document version must be greater than zero");
    }
}

}  // namespace haven::infrastructure::persistence::couchbase