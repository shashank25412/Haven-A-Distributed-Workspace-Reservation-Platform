#pragma once

#include "haven/application/idempotency/idempotency_record.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

[[nodiscard]] IdempotencyDocument to_idempotency_document(
    const haven::application::idempotency::IdempotencyRecord& record);
[[nodiscard]] haven::application::idempotency::IdempotencyRecord to_idempotency_record(
    const IdempotencyDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase
