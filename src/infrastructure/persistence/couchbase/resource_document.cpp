/**
 * @file resource_document.cpp
 * @brief Implements Couchbase resource-document JSON conversion.
 */

#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"

#include "haven/infrastructure/persistence/couchbase/resource_document_validator.hpp"
#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {

namespace {

void validate_document_type(const tao::json::value& json) {
    const auto& document_type = json.at("documentType").get_string();

    if (document_type != kResourceDocumentType) {
        throw std::invalid_argument(
            "Unexpected Couchbase document type for resource: " +
            document_type);
    }
}

}  // namespace

tao::json::value resource_document_to_json(
    const ResourceDocument& document) {
    HVN_TRACE_SCOPE();

    validate_resource_document(document);

    return tao::json::value{
        {"documentType", kResourceDocumentType},
        {"schemaVersion", document.schema_version},
        {"resourceId", document.resource_id},
        {"organizationId", document.organization_id},
        {"name", document.name},
        {"description", document.description},
        {"resourceType", document.resource_type},
        {"status", document.status},
        {"requiresApproval", document.requires_approval},
        {"version", document.version},
        {"totalUnits", document.total_units},
    };
}

ResourceDocument resource_document_from_json(
    const tao::json::value& json) {
    HVN_TRACE_SCOPE();

    validate_document_type(json);

    auto document = ResourceDocument{
        .schema_version = json.at("schemaVersion").get_unsigned(),
        .resource_id = json.at("resourceId").get_string(),
        .organization_id = json.at("organizationId").get_string(),
        .name = json.at("name").get_string(),
        .description = json.at("description").get_string(),
        .resource_type = json.at("resourceType").get_string(),
        .status = json.at("status").get_string(),
        .requires_approval = json.at("requiresApproval").get_boolean(),
        .version = json.at("version").get_unsigned(),
    };

    const auto& object = json.get_object();
    const auto total_units_field = object.find("totalUnits");
    if (total_units_field != object.end()) {
        document.total_units =
            static_cast<std::uint32_t>(total_units_field->second.get_unsigned());
    }

    validate_resource_document(document);

    return document;
}

}  // namespace haven::infrastructure::persistence::couchbase
