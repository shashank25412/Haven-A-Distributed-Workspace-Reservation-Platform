/**
 * @file idempotency_document_key.hpp
 * @brief Declares deterministic Couchbase keys for idempotency records.
 */

#pragma once

#include "haven/application/idempotency/idempotency_scope.hpp"

#include <string>

namespace haven::infrastructure::persistence::couchbase {

[[nodiscard]] std::string idempotency_document_key(
    const haven::application::idempotency::IdempotencyScope& scope);

}  // namespace haven::infrastructure::persistence::couchbase
