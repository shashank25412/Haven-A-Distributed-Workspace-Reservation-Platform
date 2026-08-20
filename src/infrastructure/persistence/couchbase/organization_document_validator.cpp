/**
 * @file organization_document_validator.cpp
 * @brief Implements validation for persisted Couchbase organization documents.
 */

#include "haven/infrastructure/persistence/couchbase/organization_document_validator.hpp"

#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

namespace {

void require_non_empty(const std::string_view value, const std::string_view field_name) {
    if (value.empty()) {
        throw std::invalid_argument(
            "Couchbase organization document field must not be empty: " + std::string{field_name});
    }
}

}  // namespace

void validate_organization_document(const OrganizationDocument& document) {
    HVN_TRACE_SCOPE();

    if (document.schema_version != kOrganizationDocumentSchemaVersion) {
        throw std::invalid_argument(
            "Unsupported Couchbase organization document schema version: " +
            std::to_string(document.schema_version));
    }

    require_non_empty(document.organization_id, "organizationId");
    require_non_empty(document.name, "name");
}

}  // namespace haven::infrastructure::persistence::couchbase
