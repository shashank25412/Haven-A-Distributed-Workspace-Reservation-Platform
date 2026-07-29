/**
 * @file resource_document_validator.hpp
 * @brief Declares validation for persisted Couchbase resource documents.
 */

#pragma once

#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Validates a deserialized Couchbase resource document.
 *
 * Validation performed here protects the persistence boundary before data is
 * converted into domain value objects and aggregates.
 *
 * @param document Resource document to validate.
 *
 * @throws std::invalid_argument If the document contains unsupported or
 * structurally invalid persisted data.
 */
void validate_resource_document(const ResourceDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase