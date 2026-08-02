/** @file test_outbox_repository.hpp @brief Configurable publisher repository test double. */
#pragma once
#include "haven/application/outbox/outbox_repository.hpp"
#include "haven/application/repository_error.hpp"

namespace haven::tests::util::application {
class TestOutboxRepository final : public haven::application::outbox::OutboxRepository {
public:
    std::vector<haven::application::outbox::LoadedOutboxMessage> pending;
    std::optional<haven::application::outbox::LoadedOutboxMessage> claim_result;
    std::optional<haven::application::outbox::LoadedOutboxMessage> mutation_result;
    std::optional<haven::application::RepositoryError> error;
    mutable std::vector<std::size_t> pending_limits;
    std::size_t claim_calls{}, published_calls{}, release_calls{};
    [[nodiscard]] std::vector<haven::application::outbox::LoadedOutboxMessage> find_pending(
        std::size_t limit) const override {
        pending_limits.push_back(limit);
        if (error)
            throw *error;
        return pending;
    }
    [[nodiscard]] std::optional<haven::application::outbox::LoadedOutboxMessage> claim(
        const haven::domain::OrganizationId&,
        const haven::domain::EventId&,
        const haven::application::persistence::PersistenceToken&) override {
        ++claim_calls;
        if (error)
            throw *error;
        return claim_result;
    }
    [[nodiscard]] haven::application::outbox::LoadedOutboxMessage mark_published(
        const haven::domain::OrganizationId&,
        const haven::domain::EventId&,
        const haven::application::persistence::PersistenceToken&,
        std::chrono::system_clock::time_point) override {
        ++published_calls;
        if (error)
            throw *error;
        return *mutation_result;
    }
    [[nodiscard]] haven::application::outbox::LoadedOutboxMessage release_for_retry(
        const haven::domain::OrganizationId&,
        const haven::domain::EventId&,
        const haven::application::persistence::PersistenceToken&) override {
        ++release_calls;
        if (error)
            throw *error;
        return *mutation_result;
    }
};
}  // namespace haven::tests::util::application
