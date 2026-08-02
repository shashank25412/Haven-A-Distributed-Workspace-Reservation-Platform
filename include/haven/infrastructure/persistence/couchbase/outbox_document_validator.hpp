/**
 * @file outbox_document_validator.hpp
 * @brief Declares validation for persisted Outbox documents.
 */

#pragma once

#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

void validate_outbox_document(const OutboxDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase
