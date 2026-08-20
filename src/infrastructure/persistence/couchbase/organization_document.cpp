/**
 * @file organization_document.cpp
 * @brief Implements Couchbase organization-document JSON conversion.
 */

#include "haven/infrastructure/persistence/couchbase/organization_document.hpp"

#include "haven/infrastructure/persistence/couchbase/organization_document_validator.hpp"
#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {

namespace {

void validate_document_type(const tao::json::value& json) {
    const auto& document_type = json.at("documentType").get_string();

    if (document_type != kOrganizationDocumentType) {
        throw std::invalid_argument(
            "Unexpected Couchbase document type for organization: " + document_type);
    }
}

}  // namespace

tao::json::value organization_document_to_json(const OrganizationDocument& document) {
    HVN_TRACE_SCOPE();

    validate_organization_document(document);

    return tao::json::value{
        {"documentType", kOrganizationDocumentType},
        {"schemaVersion", document.schema_version},
        {"organizationId", document.organization_id},
        {"name", document.name},
    };
}

OrganizationDocument organization_document_from_json(const tao::json::value& json) {
    HVN_TRACE_SCOPE();

    validate_document_type(json);

    auto document = OrganizationDocument{
        .schema_version = json.at("schemaVersion").get_unsigned(),
        .organization_id = json.at("organizationId").get_string(),
        .name = json.at("name").get_string(),
    };

    validate_organization_document(document);

    return document;
}

}  // namespace haven::infrastructure::persistence::couchbase
