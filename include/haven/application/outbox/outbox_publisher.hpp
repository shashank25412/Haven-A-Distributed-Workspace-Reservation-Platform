/**
 * @file outbox_publisher.hpp
 * @brief Declares one bounded Outbox publishing cycle.
 */
#pragma once

#include "haven/application/outbox/outbox_message_producer.hpp"
#include "haven/application/outbox/outbox_publish_cycle.hpp"
#include "haven/application/outbox/outbox_publisher_clock.hpp"
#include "haven/application/outbox/outbox_repository.hpp"

namespace haven::application::outbox {

/** @brief Sequentially coordinates one finite batch of pending Outbox messages. */
class OutboxPublisher final : public OutboxPublishCycle {
public:
    OutboxPublisher(OutboxRepository& repository,
                    OutboxMessageProducer& producer,
                    const OutboxPublisherClock& clock) noexcept;

    /**
     * @brief Processes at most one repository batch in its returned order.
     *
     * Pending-query and claim repository failures stop and propagate from the cycle. Individual
     * publication, completion, and retry-release failures are counted and processing continues.
     */
    [[nodiscard]] OutboxPublishCycleResult run_once(std::size_t batch_size) const override;

private:
    OutboxRepository& repository_;
    OutboxMessageProducer& producer_;
    const OutboxPublisherClock& clock_;
};

}  // namespace haven::application::outbox
