/**
 * @file test_outbox_message_producer.hpp
 * @brief Provides a configurable recording Outbox producer test double.
 */
#pragma once

#include "haven/application/outbox/message_publish_error.hpp"
#include "haven/application/outbox/outbox_message_producer.hpp"

#include <optional>
#include <vector>

namespace haven::application::outbox::test {

class TestOutboxMessageProducer final : public OutboxMessageProducer {
public:
    void publish(const OutboxMessage& message) override {
        published_messages.push_back(message);
        if (failure)
            throw *failure;
    }

    std::vector<OutboxMessage> published_messages;
    std::optional<MessagePublishError> failure;
};

}  // namespace haven::application::outbox::test
