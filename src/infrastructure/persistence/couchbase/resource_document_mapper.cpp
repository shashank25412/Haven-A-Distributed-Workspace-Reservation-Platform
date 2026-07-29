/**
 * @file resource_document_mapper.cpp
 * @brief Implements mapping between resources and Couchbase documents.
 */

#include "haven/infrastructure/persistence/couchbase/resource_document_mapper.hpp"

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/persistence/couchbase/resource_document_validator.hpp"
#include "haven/logging/logging.hpp"

#include <string>

namespace haven::infrastructure::persistence::couchbase {

ResourceDocument to_resource_document(const domain::Resource& resource) {
    HVN_TRACE_SCOPE();

    auto document = ResourceDocument{
        .schema_version = kResourceDocumentSchemaVersion,
        .resource_id = resource.resource_id().value(),
        .organization_id = resource.organization_id().value(),
        .name = resource.name(),
        .description = resource.description(),
        .resource_type = std::string{domain::to_string(resource.type())},
        .status = std::string{domain::to_string(resource.status())},
        .requires_approval = resource.requires_approval(),
        .version = resource.version().value(),
    };

    validate_resource_document(document);
    return document;
}

domain::Resource to_domain_resource(const ResourceDocument& document) {
    HVN_TRACE_SCOPE();

    validate_resource_document(document);

    return domain::Resource::rehydrate(
        domain::OrganizationId{document.organization_id},
        domain::ResourceId{document.resource_id},
        document.name,
        document.description,
        domain::resource_type_from_string(document.resource_type),
        domain::resource_status_from_string(document.status),
        document.requires_approval,
        domain::Version{document.version});
}

}  // namespace haven::infrastructure::persistence::couchbase
