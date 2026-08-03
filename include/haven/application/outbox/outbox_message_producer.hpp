/**
 * @file outbox_message_producer.hpp
 * @brief Declares the transport-neutral acknowledged message producer port.
 */
#pragma once

#include "haven/application/outbox/outbox_message.hpp"

namespace haven::application::outbox {

class OutboxMessageProducer {
public:
    virtual ~OutboxMessageProducer() = default;

    /** @brief Returns only after the configured broker acknowledgement is received. */
    virtual void publish(const OutboxMessage& message) = 0;
};

}  // namespace haven::application::outbox
