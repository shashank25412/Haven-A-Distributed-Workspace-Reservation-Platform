/**
 * @file outbox_repository.hpp
 * @brief Defines publisher-side durable Outbox state transitions.
 */
#pragma once

#include "haven/application/outbox/outbox_message.hpp"
#include "haven/application/persistence/loaded.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

namespace haven::application::outbox {

using LoadedOutboxMessage = haven::application::persistence::Loaded<OutboxMessage>;

class OutboxRepository {
public:
    virtual ~OutboxRepository() = default;
    [[nodiscard]] virtual std::vector<LoadedOutboxMessage> find_pending(
        std::size_t limit) const = 0;
    [[nodiscard]] virtual std::optional<LoadedOutboxMessage> claim(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const haven::application::persistence::PersistenceToken& expected_token) = 0;
    [[nodiscard]] virtual LoadedOutboxMessage mark_published(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const haven::application::persistence::PersistenceToken& expected_token,
        std::chrono::system_clock::time_point published_at) = 0;
    [[nodiscard]] virtual LoadedOutboxMessage release_for_retry(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const haven::application::persistence::PersistenceToken& expected_token) = 0;
};

}  // namespace haven::application::outbox
