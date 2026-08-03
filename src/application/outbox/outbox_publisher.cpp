/**
 * @file outbox_publisher.cpp
 * @brief Implements one bounded Outbox publishing cycle.
 */
#include "haven/application/outbox/outbox_publisher.hpp"

#include "haven/application/outbox/message_publish_error.hpp"
#include "haven/application/repository_error.hpp"
#include "haven/logging/logging.hpp"

#include <stdexcept>

namespace haven::application::outbox {

OutboxPublisher::OutboxPublisher(OutboxRepository& repository,
                                 OutboxMessageProducer& producer,
                                 const OutboxPublisherClock& clock) noexcept
    : repository_(repository), producer_(producer), clock_(clock) {}

OutboxPublishCycleResult OutboxPublisher::run_once(const std::size_t batch_size) const {
    HVN_TRACE_SCOPE();
    if (batch_size == 0)
        throw std::invalid_argument("Outbox publisher batch size must be positive");
    HVN_DEBUG_LOG("Starting Outbox publish cycle with batch size ", batch_size);

    auto result = OutboxPublishCycleResult{};
    std::vector<LoadedOutboxMessage> candidates;
    try {
        candidates = repository_.find_pending(batch_size);
    } catch (const haven::application::RepositoryError&) {
        HVN_ERROR_LOG("Outbox pending query prevented publish-cycle progress");
        throw;
    }
    result.candidates_found = candidates.size();

    for (const auto& candidate : candidates) {
        const auto& pending = candidate.aggregate();
        std::optional<LoadedOutboxMessage> claimed;
        try {
            claimed = repository_.claim(
                pending.organization_id, pending.event_id, candidate.persistence_token());
        } catch (const haven::application::RepositoryError&) {
            HVN_ERROR_LOG("Outbox claim prevented publish-cycle progress for event ",
                          pending.event_id.value());
            throw;
        }
        if (!claimed) {
            ++result.claims_lost;
            HVN_DEBUG_LOG("Outbox claim lost for event ", pending.event_id.value());
            continue;
        }
        ++result.claims_acquired;
        const auto& message = claimed->aggregate();
        try {
            producer_.publish(message);
        } catch (const MessagePublishError&) {
            HVN_WARN_LOG("Outbox publication failed for event ", message.event_id.value());
            try {
                static_cast<void>(repository_.release_for_retry(
                    message.organization_id, message.event_id, claimed->persistence_token()));
                ++result.released_for_retry;
            } catch (const haven::application::RepositoryError&) {
                ++result.release_failures;
                HVN_WARN_LOG("Outbox retry release failed for event ", message.event_id.value());
            }
            continue;
        }

        try {
            static_cast<void>(repository_.mark_published(message.organization_id,
                                                         message.event_id,
                                                         claimed->persistence_token(),
                                                         clock_.now()));
            ++result.published;
            HVN_DEBUG_LOG("Outbox message published for event ", message.event_id.value());
        } catch (const haven::application::RepositoryError&) {
            ++result.completion_failures;
            HVN_WARN_LOG("Outbox completion failed after acknowledgement for event ",
                         message.event_id.value());
        }
    }

    HVN_DEBUG_LOG("Completed Outbox publish cycle: candidates=",
                  result.candidates_found,
                  ", claimed=",
                  result.claims_acquired,
                  ", published=",
                  result.published);
    return result;
}

}  // namespace haven::application::outbox
