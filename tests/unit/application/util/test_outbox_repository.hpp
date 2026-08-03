/**
 * @file test_outbox_repository.hpp
 * @brief Provides a scriptable recording Outbox repository test double.
 */
#pragma once

#include "haven/application/outbox/outbox_repository.hpp"
#include "haven/application/repository_error.hpp"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace haven::tests::util::application {

class TestOutboxRepository final : public haven::application::outbox::OutboxRepository {
public:
    using LoadedMessage = haven::application::outbox::LoadedOutboxMessage;
    using Token = haven::application::persistence::PersistenceToken;

    struct ClaimBehavior {
        std::optional<LoadedMessage> result;
        std::optional<haven::application::RepositoryError> error;
    };
    struct MutationBehavior {
        std::optional<LoadedMessage> result;
        std::optional<haven::application::RepositoryError> error;
    };
    struct ClaimCall {
        haven::domain::OrganizationId organization_id;
        haven::domain::EventId event_id;
        Token token;
    };
    struct MutationCall {
        haven::domain::OrganizationId organization_id;
        haven::domain::EventId event_id;
        Token token;
        std::optional<std::chrono::system_clock::time_point> published_at;
    };

    std::vector<LoadedMessage> pending;
    std::optional<haven::application::RepositoryError> pending_error;
    std::deque<ClaimBehavior> claim_behaviors;
    std::deque<MutationBehavior> mark_behaviors;
    std::deque<MutationBehavior> release_behaviors;
    mutable std::vector<std::size_t> pending_limits;
    std::vector<ClaimCall> claim_calls;
    std::vector<MutationCall> mark_calls;
    std::vector<MutationCall> release_calls;
    std::vector<std::string>* call_order{};

    [[nodiscard]] std::vector<LoadedMessage> find_pending(std::size_t limit) const override {
        pending_limits.push_back(limit);
        record("find_pending");
        if (pending_error)
            throw *pending_error;
        return pending;
    }

    [[nodiscard]] std::optional<LoadedMessage> claim(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const Token& token) override {
        claim_calls.push_back({organization_id, event_id, token});
        record("claim:" + event_id.value());
        if (claim_behaviors.empty())
            return std::nullopt;
        auto behavior = std::move(claim_behaviors.front());
        claim_behaviors.pop_front();
        if (behavior.error)
            throw *behavior.error;
        return std::move(behavior.result);
    }

    [[nodiscard]] LoadedMessage mark_published(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const Token& token,
        std::chrono::system_clock::time_point published_at) override {
        mark_calls.push_back({organization_id, event_id, token, published_at});
        record("mark:" + event_id.value());
        return mutate(mark_behaviors);
    }

    [[nodiscard]] LoadedMessage release_for_retry(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::EventId& event_id,
        const Token& token) override {
        release_calls.push_back({organization_id, event_id, token, std::nullopt});
        record("release:" + event_id.value());
        return mutate(release_behaviors);
    }

private:
    void record(std::string operation) const {
        if (call_order)
            call_order->push_back(std::move(operation));
    }

    [[nodiscard]] static LoadedMessage mutate(std::deque<MutationBehavior>& behaviors) {
        if (behaviors.empty())
            throw std::logic_error("No scripted Outbox mutation behavior");
        auto behavior = std::move(behaviors.front());
        behaviors.pop_front();
        if (behavior.error)
            throw *behavior.error;
        if (!behavior.result)
            throw std::logic_error("Scripted Outbox mutation has no result");
        return std::move(*behavior.result);
    }
};

}  // namespace haven::tests::util::application
