/**
 * @file couchbase_outbox_repository.hpp
 * @brief Declares the Couchbase publisher-side Outbox repository.
 */
#pragma once

#include "haven/application/outbox/outbox_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include <memory>

namespace haven::infrastructure::persistence::couchbase {
class CouchbaseOutboxRepository final : public haven::application::outbox::OutboxRepository {
public:
    explicit CouchbaseOutboxRepository(std::shared_ptr<CouchbaseConnection> connection);
    [[nodiscard]] std::vector<haven::application::outbox::LoadedOutboxMessage> find_pending(
        std::size_t limit) const override;
    [[nodiscard]] std::optional<haven::application::outbox::LoadedOutboxMessage> claim(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const haven::application::persistence::PersistenceToken& expected_token) override;
    [[nodiscard]] haven::application::outbox::LoadedOutboxMessage mark_published(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const haven::application::persistence::PersistenceToken& expected_token,
        std::chrono::system_clock::time_point published_at) override;
    [[nodiscard]] haven::application::outbox::LoadedOutboxMessage release_for_retry(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const haven::application::persistence::PersistenceToken& expected_token) override;

private:
    std::shared_ptr<CouchbaseConnection> connection_;
};
}  // namespace haven::infrastructure::persistence::couchbase
