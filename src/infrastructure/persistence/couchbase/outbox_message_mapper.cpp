/** @file outbox_message_mapper.cpp */
#include "haven/infrastructure/persistence/couchbase/outbox_message_mapper.hpp"

#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <tao/json.hpp>

namespace haven::infrastructure::persistence::couchbase {

haven::application::outbox::OutboxMessage to_outbox_message(const OutboxDocument& document) {
    const auto envelope =
        tao::json::value{{"schemaVersion", document.schema_version},
                         {"eventId", document.event_id.value()},
                         {"organizationId", document.organization_id.value()},
                         {"aggregateId", document.aggregate_id.value()},
                         {"aggregateType", document.aggregate_type},
                         {"eventType", document.event_type},
                         {"occurredAt", reservation_timestamp_to_string(document.occurred_at)},
                         {"payload", document.payload}};
    return {.event_id = document.event_id,
            .organization_id = document.organization_id,
            .aggregate_id = document.aggregate_id,
            .aggregate_type = document.aggregate_type,
            .event_type = document.event_type,
            .occurred_at = document.occurred_at,
            .schema_version = document.schema_version,
            .serialized_envelope = tao::json::to_string(envelope),
            .attempt_count = document.attempt_count};
}

}  // namespace haven::infrastructure::persistence::couchbase
