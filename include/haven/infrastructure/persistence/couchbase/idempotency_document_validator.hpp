/**
 * @file idempotency_document_validator.hpp
 * @brief Declares validation for persisted Couchbase idempotency documents.
 */

#pragma once

#include "haven/application/idempotency/idempotency_scope.hpp"
#include "haven/application/idempotency/idempotency_status.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document.hpp"

#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

[[nodiscard]] std::string_view idempotency_operation_to_string(
    haven::application::idempotency::IdempotencyOperation operation);
[[nodiscard]] haven::application::idempotency::IdempotencyOperation
idempotency_operation_from_string(std::string_view value);
[[nodiscard]] std::string_view idempotency_status_to_string(
    haven::application::idempotency::IdempotencyStatus status);
[[nodiscard]] haven::application::idempotency::IdempotencyStatus idempotency_status_from_string(
    std::string_view value);
void validate_idempotency_document(const IdempotencyDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase
