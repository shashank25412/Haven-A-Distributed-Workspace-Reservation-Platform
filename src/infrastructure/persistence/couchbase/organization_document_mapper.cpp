/**
 * @file organization_document_mapper.cpp
 * @brief Implements mapping between organizations and Couchbase documents.
 */

#include "haven/infrastructure/persistence/couchbase/organization_document_mapper.hpp"

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/infrastructure/persistence/couchbase/organization_document_validator.hpp"
#include "haven/logging/logging.hpp"

namespace haven::infrastructure::persistence::couchbase {

OrganizationDocument to_organization_document(const domain::Organization& organization) {
    HVN_TRACE_SCOPE();

    auto document = OrganizationDocument{
        .schema_version = kOrganizationDocumentSchemaVersion,
        .organization_id = organization.organization_id().value(),
        .name = organization.name(),
    };

    validate_organization_document(document);
    return document;
}

domain::Organization to_domain_organization(const OrganizationDocument& document) {
    HVN_TRACE_SCOPE();

    validate_organization_document(document);

    return domain::Organization{domain::OrganizationId{document.organization_id}, document.name};
}

}  // namespace haven::infrastructure::persistence::couchbase
