/**
 * @file organization_document_validator.hpp
 * @brief Declares validation for persisted Couchbase organization documents.
 */

#pragma once

#include "haven/infrastructure/persistence/couchbase/organization_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Validates a deserialized Couchbase organization document.
 *
 * @throws std::invalid_argument If the document contains unsupported or
 * structurally invalid persisted data.
 */
void validate_organization_document(const OrganizationDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase
