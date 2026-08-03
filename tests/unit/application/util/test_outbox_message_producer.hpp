/**
 * @file test_outbox_message_producer.hpp
 * @brief Provides a configurable recording Outbox producer test double.
 */
#pragma once

#include "haven/application/outbox/message_publish_error.hpp"
#include "haven/application/outbox/outbox_message_producer.hpp"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace haven::application::outbox::test {

class TestOutboxMessageProducer final : public OutboxMessageProducer {
public:
    void publish(const OutboxMessage& message) override {
        published_messages.push_back(message);
        if (call_order)
            call_order->push_back("publish:" + message.event_id.value());
        if (!failures.empty()) {
            auto failure = std::move(failures.front());
            failures.pop_front();
            if (failure)
                throw *failure;
        } else if (this->failure) {
            throw *this->failure;
        }
    }

    std::vector<OutboxMessage> published_messages;
    std::optional<MessagePublishError> failure;
    std::deque<std::optional<MessagePublishError>> failures;
    std::vector<std::string>* call_order{};
};

}  // namespace haven::application::outbox::test
