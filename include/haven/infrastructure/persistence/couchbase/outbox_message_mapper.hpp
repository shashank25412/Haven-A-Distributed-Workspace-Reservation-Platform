/**
 * @file outbox_message_mapper.hpp
 * @brief Maps Couchbase Outbox documents to transport-neutral messages.
 */
#pragma once

#include "haven/application/outbox/outbox_message.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"

namespace haven::infrastructure::persistence::couchbase {
[[nodiscard]] haven::application::outbox::OutboxMessage to_outbox_message(
    const OutboxDocument& document);
}
