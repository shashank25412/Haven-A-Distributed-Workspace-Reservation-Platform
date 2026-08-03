/**
 * @file outbox_publisher_test.cpp
 * @brief Tests one-cycle Outbox publication orchestration.
 */
#include "haven/application/outbox/outbox_publisher.hpp"

#include "haven/application/repository_error.hpp"

#include "application/util/recording_metrics_recorder.hpp"
#include "application/util/test_outbox_message_producer.hpp"
#include "application/util/test_outbox_publisher_clock.hpp"
#include "application/util/test_outbox_repository.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace haven::application::outbox {
namespace {
using Repository = haven::tests::util::application::TestOutboxRepository;
using Producer = haven::application::outbox::test::TestOutboxMessageProducer;
using Clock = haven::application::outbox::test::TestOutboxPublisherClock;
using Token = haven::application::persistence::PersistenceToken;
using Metrics = haven::test::application::observability::metrics::RecordingMetricsRecorder;

class ThrowingMetrics final : public haven::application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(
        const haven::application::observability::metrics::MetricName&,
        double,
        const haven::application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
    void set_gauge(const haven::application::observability::metrics::MetricName&,
                   double,
                   const haven::application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
    void observe_duration(
        const haven::application::observability::metrics::MetricName&,
        std::chrono::microseconds,
        const haven::application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
};

OutboxMessage message(std::string event, std::string envelope = "pending") {
    return {.event_id = haven::domain::EventId{std::move(event)},
            .organization_id = haven::domain::OrganizationId{"organization"},
            .aggregate_id = haven::domain::ReservationId{"reservation"},
            .aggregate_type = "Reservation",
            .event_type = "ReservationCreated",
            .occurred_at = {},
            .schema_version = 1,
            .serialized_envelope = std::move(envelope),
            .attempt_count = 0};
}

LoadedOutboxMessage loaded(std::string event,
                           std::uint64_t token,
                           std::string envelope = "pending") {
    return {message(std::move(event), std::move(envelope)), Token{token}};
}

haven::application::RepositoryError repository_error() {
    return {haven::application::RepositoryErrorCode::Persistence, "repository failure"};
}

Repository::MutationBehavior mutation(LoadedOutboxMessage result) {
    return {.result = std::move(result), .error = std::nullopt};
}

Repository::MutationBehavior mutation_error() {
    return {.result = std::nullopt, .error = repository_error()};
}

class OutboxPublisherTest : public testing::Test {
protected:
    void SetUp() override {
        repository.call_order = &order;
        producer.call_order = &order;
        clock.current = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}};
    }

    Repository repository;
    Producer producer;
    Clock clock;
    Metrics metrics;
    std::vector<std::string> order;
};
}  // namespace

TEST_F(OutboxPublisherTest, EmptyBatchHasNoDownstreamInteractions) {
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(10);
    EXPECT_EQ(result, OutboxPublishCycleResult{});
    EXPECT_EQ(repository.pending_limits, std::vector<std::size_t>{10});
    EXPECT_TRUE(repository.claim_calls.empty());
    EXPECT_TRUE(producer.published_messages.empty());
    EXPECT_EQ(order, std::vector<std::string>{"find_pending"});
    ASSERT_EQ(metrics.counter_increments().size(), 2U);
    EXPECT_EQ(metrics.counter_increments()[0].name.value(),
              "haven_outbox_publisher_records_discovered_total");
    EXPECT_EQ(metrics.counter_increments()[0].amount, 0.0);
    EXPECT_EQ(metrics.counter_increments()[1].labels[0].value(), "completed");
    ASSERT_EQ(metrics.duration_observations().size(), 1U);
    EXPECT_EQ(metrics.duration_observations()[0].labels[0].value(), "completed");
}

TEST_F(OutboxPublisherTest, PublishesClaimedMessageWithRotatedTokenAndClockTime) {
    repository.pending.push_back(loaded("event", 11));
    repository.claim_behaviors.push_back({loaded("event", 12, "claimed"), std::nullopt});
    repository.mark_behaviors.push_back(mutation(loaded("event", 13, "claimed")));
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(1);
    EXPECT_EQ(
        result,
        (OutboxPublishCycleResult{.candidates_found = 1, .claims_acquired = 1, .published = 1}));
    ASSERT_EQ(producer.published_messages.size(), 1U);
    EXPECT_EQ(producer.published_messages[0].serialized_envelope, "claimed");
    ASSERT_EQ(repository.claim_calls.size(), 1U);
    EXPECT_EQ(repository.claim_calls[0].token, Token{11});
    ASSERT_EQ(repository.mark_calls.size(), 1U);
    EXPECT_EQ(repository.mark_calls[0].token, Token{12});
    EXPECT_EQ(repository.mark_calls[0].published_at, clock.current);
    EXPECT_TRUE(repository.release_calls.empty());
    EXPECT_EQ(clock.calls, 1U);
    EXPECT_EQ(
        order,
        (std::vector<std::string>{"find_pending", "claim:event", "publish:event", "mark:event"}));
}

TEST_F(OutboxPublisherTest, LostClaimIsCountedWithoutPublication) {
    repository.pending.push_back(loaded("event", 11));
    repository.claim_behaviors.push_back({std::nullopt, std::nullopt});
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(1);
    EXPECT_EQ(result.candidates_found, 1U);
    EXPECT_EQ(result.claims_lost, 1U);
    EXPECT_EQ(result.claims_acquired, 0U);
    EXPECT_TRUE(producer.published_messages.empty());
    EXPECT_TRUE(repository.mark_calls.empty());
    EXPECT_TRUE(repository.release_calls.empty());
}

TEST_F(OutboxPublisherTest, PreservesPendingCandidateOrder) {
    repository.pending = {loaded("first", 11), loaded("second", 21)};
    repository.claim_behaviors.push_back({loaded("first", 12), std::nullopt});
    repository.claim_behaviors.push_back({loaded("second", 22), std::nullopt});
    repository.mark_behaviors.push_back(mutation(loaded("first", 13)));
    repository.mark_behaviors.push_back(mutation(loaded("second", 23)));
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(2);
    EXPECT_EQ(result.published, 2U);
    ASSERT_EQ(producer.published_messages.size(), 2U);
    EXPECT_EQ(producer.published_messages[0].event_id, haven::domain::EventId{"first"});
    EXPECT_EQ(producer.published_messages[1].event_id, haven::domain::EventId{"second"});
}

TEST_F(OutboxPublisherTest, PublishFailureReleasesWithRotatedTokenAndContinues) {
    repository.pending = {loaded("failed", 11), loaded("next", 21)};
    repository.claim_behaviors.push_back({loaded("failed", 12), std::nullopt});
    repository.claim_behaviors.push_back({loaded("next", 22), std::nullopt});
    repository.release_behaviors.push_back(mutation(loaded("failed", 13)));
    repository.mark_behaviors.push_back(mutation(loaded("next", 23)));
    producer.failures.push_back(MessagePublishError{MessagePublishErrorCode::Unavailable, "down"});
    producer.failures.push_back(std::nullopt);
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(2);
    EXPECT_EQ(result.released_for_retry, 1U);
    EXPECT_EQ(result.published, 1U);
    ASSERT_EQ(repository.release_calls.size(), 1U);
    EXPECT_EQ(repository.release_calls[0].token, Token{12});
    EXPECT_EQ(order,
              (std::vector<std::string>{"find_pending",
                                        "claim:failed",
                                        "publish:failed",
                                        "release:failed",
                                        "claim:next",
                                        "publish:next",
                                        "mark:next"}));
}

TEST_F(OutboxPublisherTest, ReleaseFailureIsCountedAndCycleContinues) {
    repository.pending = {loaded("failed", 11), loaded("next", 21)};
    repository.claim_behaviors.push_back({loaded("failed", 12), std::nullopt});
    repository.claim_behaviors.push_back({loaded("next", 22), std::nullopt});
    repository.release_behaviors.push_back(mutation_error());
    repository.mark_behaviors.push_back(mutation(loaded("next", 23)));
    producer.failures.push_back(MessagePublishError{MessagePublishErrorCode::Timeout, "timeout"});
    producer.failures.push_back(std::nullopt);
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(2);
    EXPECT_EQ(result.release_failures, 1U);
    EXPECT_EQ(result.released_for_retry, 0U);
    EXPECT_EQ(result.published, 1U);
}

TEST_F(OutboxPublisherTest, CompletionFailureNeverReleasesAndCycleContinues) {
    repository.pending = {loaded("ambiguous", 11), loaded("next", 21)};
    repository.claim_behaviors.push_back({loaded("ambiguous", 12), std::nullopt});
    repository.claim_behaviors.push_back({loaded("next", 22), std::nullopt});
    repository.mark_behaviors.push_back(mutation_error());
    repository.mark_behaviors.push_back(mutation(loaded("next", 23)));
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(2);
    EXPECT_EQ(result.completion_failures, 1U);
    EXPECT_EQ(result.published, 1U);
    EXPECT_TRUE(repository.release_calls.empty());
    EXPECT_EQ(producer.published_messages.size(), 2U);
}

TEST_F(OutboxPublisherTest, ClaimRepositoryFailurePropagatesAndStopsLaterCandidates) {
    repository.pending = {loaded("first", 11), loaded("later", 21)};
    repository.claim_behaviors.push_back({std::nullopt, repository_error()});
    EXPECT_THROW(
        static_cast<void>(OutboxPublisher(repository, producer, clock, metrics).run_once(2)),
        haven::application::RepositoryError);
    EXPECT_EQ(repository.claim_calls.size(), 1U);
    EXPECT_TRUE(producer.published_messages.empty());
    ASSERT_EQ(metrics.counter_increments().size(), 3U);
    EXPECT_EQ(metrics.counter_increments()[1].labels[0].value(), "claim_failed");
    EXPECT_EQ(metrics.counter_increments()[2].labels[0].value(), "repository_failure");
}

TEST_F(OutboxPublisherTest, PendingQueryFailurePropagatesWithoutDownstreamInteraction) {
    repository.pending_error = repository_error();
    EXPECT_THROW(
        static_cast<void>(OutboxPublisher(repository, producer, clock, metrics).run_once(2)),
        haven::application::RepositoryError);
    EXPECT_TRUE(repository.claim_calls.empty());
    EXPECT_TRUE(producer.published_messages.empty());
    ASSERT_EQ(metrics.counter_increments().size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].labels[0].value(), "repository_failure");
}

TEST_F(OutboxPublisherTest, ZeroBatchSizeIsRejectedBeforeRepositoryQuery) {
    EXPECT_THROW(
        static_cast<void>(OutboxPublisher(repository, producer, clock, metrics).run_once(0)),
        std::invalid_argument);
    EXPECT_TRUE(repository.pending_limits.empty());
    ASSERT_EQ(metrics.counter_increments().size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].labels[0].value(), "unexpected_failure");
}

TEST_F(OutboxPublisherTest, MixedBatchReportsEveryOutcome) {
    repository.pending = {
        loaded("success", 11), loaded("lost", 21), loaded("retry", 31), loaded("completion", 41)};
    repository.claim_behaviors.push_back({loaded("success", 12), std::nullopt});
    repository.claim_behaviors.push_back({std::nullopt, std::nullopt});
    repository.claim_behaviors.push_back({loaded("retry", 32), std::nullopt});
    repository.claim_behaviors.push_back({loaded("completion", 42), std::nullopt});
    repository.mark_behaviors.push_back(mutation(loaded("success", 13)));
    repository.mark_behaviors.push_back(mutation_error());
    repository.release_behaviors.push_back(mutation(loaded("retry", 33)));
    producer.failures.push_back(std::nullopt);
    producer.failures.push_back(MessagePublishError{MessagePublishErrorCode::Timeout, "timeout"});
    producer.failures.push_back(std::nullopt);
    const auto result = OutboxPublisher{repository, producer, clock, metrics}.run_once(4);
    EXPECT_EQ(result,
              (OutboxPublishCycleResult{.candidates_found = 4,
                                        .claims_acquired = 3,
                                        .claims_lost = 1,
                                        .published = 1,
                                        .released_for_retry = 1,
                                        .completion_failures = 1,
                                        .release_failures = 0}));
    ASSERT_EQ(metrics.counter_increments().size(), 6U);
    EXPECT_EQ(metrics.counter_increments()[0].amount, 4.0);
    EXPECT_EQ(metrics.counter_increments()[1].labels[0].value(), "published");
    EXPECT_EQ(metrics.counter_increments()[2].labels[0].value(), "claim_not_acquired");
    EXPECT_EQ(metrics.counter_increments()[3].labels[0].value(), "publish_failed_released");
    EXPECT_EQ(metrics.counter_increments()[4].labels[0].value(), "published_mark_failed");
    EXPECT_EQ(metrics.counter_increments()[5].labels[0].value(), "completed");
}

TEST_F(OutboxPublisherTest, MetricsFailuresDoNotAlterPublishing) {
    repository.pending.push_back(loaded("event", 11));
    repository.claim_behaviors.push_back({loaded("event", 12), std::nullopt});
    repository.mark_behaviors.push_back(mutation(loaded("event", 13)));
    auto throwing_metrics = ThrowingMetrics{};

    const auto result = OutboxPublisher{repository, producer, clock, throwing_metrics}.run_once(1);

    EXPECT_EQ(result.published, 1U);
}

}  // namespace haven::application::outbox
