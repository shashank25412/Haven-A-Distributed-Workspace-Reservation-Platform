/**
 * @file create_reservation_handler_test.cpp
 * @brief Tests CreateReservation application orchestration.
 */

#include "haven/application/reservations/create_reservation_handler.hpp"

#include "haven/application/idempotency/create_reservation_fingerprint_input.hpp"
#include "haven/application/repository_error.hpp"
#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/policies/reservation_creation_policy.hpp"
#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/idempotency_key.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include "application/idempotency/test_idempotency_repository.hpp"
#include "application/util/recording_metrics_recorder.hpp"
#include "application/util/test_reservation_creation_event_store.hpp"
#include "application/util/test_reservation_creation_store.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace haven::application::reservations {
namespace {

using haven::application::resources::ResourceLookupResult;
using haven::application::resources::ResourceRepository;
using haven::application::resources::ResourceSearchResult;
using haven::domain::EventId;
using haven::domain::IdempotencyKey;
using haven::domain::OrganizationId;
using haven::domain::Purpose;
using haven::domain::Reservation;
using haven::domain::ReservationCreationPolicy;
using haven::domain::ReservationId;
using haven::domain::ReservationKind;
using haven::domain::ReservationStatus;
using haven::domain::Resource;
using haven::domain::ResourceId;
using haven::domain::ResourceType;
using haven::domain::TimeInterval;
using haven::domain::UserId;

using namespace std::chrono_literals;
using haven::test::application::idempotency::TestIdempotencyRepository;
using haven::test::application::observability::metrics::RecordingMetricsRecorder;

class InMemoryResourceRepository final : public ResourceRepository {
public:
    void add(Resource resource) {
        resources_.push_back(std::move(resource));
    }

    void force_unexpected_failure() noexcept {
        force_unexpected_failure_ = true;
    }

    [[nodiscard]] ResourceLookupResult find_by_id(const OrganizationId& organization_id,
                                                  const ResourceId& resource_id) const override {
        ++find_call_count_;
        if (force_unexpected_failure_)
            throw std::runtime_error("forced unexpected resource failure");
        const auto resource =
            std::find_if(resources_.cbegin(),
                         resources_.cend(),
                         [&organization_id, &resource_id](const Resource& candidate) {
                             return candidate.organization_id() == organization_id &&
                                    candidate.resource_id() == resource_id;
                         });

        if (resource == resources_.cend()) {
            return std::nullopt;
        }

        return haven::application::resources::LoadedResource{
            *resource, haven::application::persistence::PersistenceToken{1}};
    }

    [[nodiscard]] std::size_t find_call_count() const noexcept {
        return find_call_count_.load();
    }

    [[nodiscard]] ResourceSearchResult find_active_by_type(const OrganizationId&,
                                                           ResourceType) const override {
        return {};
    }

private:
    std::vector<Resource> resources_;
    bool force_unexpected_failure_{false};
    mutable std::atomic_size_t find_call_count_{};
};

class InMemoryReservationRepository final : public ReservationRepository {
public:
    void set_conflict(const bool conflict) noexcept {
        conflict_ = conflict;
    }

    void store(Reservation reservation) {
        saved_reservation_ = std::move(reservation);
    }

    [[nodiscard]] bool has_conflict(const OrganizationId&,
                                    const ResourceId&,
                                    const TimeInterval&) const override {
        ++conflict_call_count_;
        return conflict_;
    }

    [[nodiscard]] bool has_conflict_excluding(const OrganizationId&,
                                              const ResourceId&,
                                              const TimeInterval&,
                                              const ReservationId&) const override {
        return false;
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const OrganizationId& organization_id, const Reservation& reservation) override {
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
        ++insert_call_count_;
        return haven::application::persistence::PersistenceToken{1};
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const OrganizationId&,
        const Reservation&,
        const haven::application::persistence::PersistenceToken&) override {
        return haven::application::persistence::PersistenceToken{2};
    }

    [[nodiscard]] const std::optional<OrganizationId>& saved_organization_id() const noexcept {
        return saved_organization_id_;
    }

    [[nodiscard]] const std::optional<Reservation>& saved_reservation() const noexcept {
        return saved_reservation_;
    }

    [[nodiscard]] ReservationLookupResult find_by_id(
        const OrganizationId& organization_id, const ReservationId& reservation_id) const override {
        ++find_call_count_;
        if (saved_reservation_ && saved_reservation_->organization_id() == organization_id &&
            saved_reservation_->reservation_id() == reservation_id) {
            return haven::application::reservations::LoadedReservation{
                *saved_reservation_, haven::application::persistence::PersistenceToken{1}};
        }
        return std::nullopt;
    }

    [[nodiscard]] std::size_t conflict_call_count() const noexcept {
        return conflict_call_count_;
    }
    [[nodiscard]] std::size_t insert_call_count() const noexcept {
        return insert_call_count_;
    }
    [[nodiscard]] std::size_t find_call_count() const noexcept {
        return find_call_count_.load();
    }

    [[nodiscard]] ReservationListResult find_by_creator(const OrganizationId&,
                                                        const UserId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
        const OrganizationId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_decided_approvals(
        const OrganizationId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_by_resource_and_interval(
        const OrganizationId&, const ResourceId&, const TimeInterval&) const override {
        return {};
    }

private:
    bool conflict_{false};
    std::optional<OrganizationId> saved_organization_id_;
    std::optional<Reservation> saved_reservation_;
    mutable std::size_t conflict_call_count_{};
    std::size_t insert_call_count_{};
    mutable std::atomic_size_t find_call_count_{};
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] TimeInterval make_interval() {
    return TimeInterval{make_time_point(10), make_time_point(11)};
}

[[nodiscard]] Resource make_resource(const ResourceId& resource_id,
                                     const OrganizationId& organization_id,
                                     const bool requires_approval) {
    return Resource{organization_id,
                    resource_id,
                    ResourceType::MeetingRoom,
                    haven::domain::ResourceStatus::Active,
                    requires_approval};
}

[[nodiscard]] CreateReservationCommand make_command(
    const OrganizationId& organization_id,
    const ResourceId& resource_id,
    const ReservationKind reservation_kind = ReservationKind::Standard,
    const bool maintenance_authorized = false) {
    return CreateReservationCommand{organization_id,
                                    IdempotencyKey{"idempotency-key-100"},
                                    ReservationId{"reservation-100"},
                                    resource_id,
                                    UserId{"user-100"},
                                    make_interval(),
                                    Purpose{"Planning meeting"},
                                    reservation_kind,
                                    maintenance_authorized,
                                    EventId{"event-created-100"},
                                    EventId{"event-confirmed-100"},
                                    EventId{"event-approval-100"},
                                    make_time_point(9)};
}

[[nodiscard]] haven::application::idempotency::IdempotencyRecord processing_for(
    const CreateReservationCommand& command) {
    using namespace haven::application::idempotency;
    return IdempotencyRecord::processing(
        IdempotencyScope{command.organization_id(),
                         command.creator_id(),
                         IdempotencyOperation::CreateReservation,
                         command.idempotency_key()},
        create_reservation_fingerprint(
            CreateReservationFingerprintInput{command.resource_id(),
                                              command.creator_id(),
                                              command.interval(),
                                              command.purpose(),
                                              command.reservation_kind(),
                                              command.maintenance_authorized()}),
        {command.reservation_id(),
         command.created_event_id(),
         command.confirmed_event_id(),
         command.approval_requested_event_id()},
        command.occurred_at());
}

void expect_metrics(const RecordingMetricsRecorder& recorder,
                    const std::vector<std::string>& expected_outcomes) {
    ASSERT_EQ(recorder.counter_increments().size(), expected_outcomes.size());
    ASSERT_EQ(recorder.duration_observations().size(), expected_outcomes.size());
    for (std::size_t index = 0; index < expected_outcomes.size(); ++index) {
        const auto& counter = recorder.counter_increments()[index];
        const auto& duration = recorder.duration_observations()[index];
        EXPECT_EQ(counter.name.value(), "haven_reservation_creation_attempts_total");
        EXPECT_EQ(counter.amount, 1.0);
        ASSERT_EQ(counter.labels.size(), 1U);
        EXPECT_EQ(counter.labels.front().name(), "outcome");
        EXPECT_EQ(counter.labels.front().value(), expected_outcomes[index]);
        EXPECT_EQ(duration.name.value(), "haven_reservation_creation_duration_seconds");
        EXPECT_GE(duration.duration.count(), 0);
        EXPECT_EQ(duration.labels, counter.labels);
    }
}

class ThrowingMetricsRecorder final
    : public haven::application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(
        const haven::application::observability::metrics::MetricName&,
        double,
        const haven::application::observability::metrics::MetricLabels&) override {
        ++counter_calls;
        throw std::runtime_error("forced counter failure");
    }

    void set_gauge(const haven::application::observability::metrics::MetricName&,
                   double,
                   const haven::application::observability::metrics::MetricLabels&) override {}

    void observe_duration(
        const haven::application::observability::metrics::MetricName&,
        std::chrono::microseconds,
        const haven::application::observability::metrics::MetricLabels&) override {
        ++duration_calls;
        throw std::runtime_error("forced duration failure");
    }

    std::size_t counter_calls{};
    std::size_t duration_calls{};
};

class DiscardingMetricsRecorder final
    : public haven::application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(
        const haven::application::observability::metrics::MetricName&,
        double,
        const haven::application::observability::metrics::MetricLabels&) override {}

    void set_gauge(const haven::application::observability::metrics::MetricName&,
                   double,
                   const haven::application::observability::metrics::MetricLabels&) override {}

    void observe_duration(
        const haven::application::observability::metrics::MetricName&,
        std::chrono::microseconds,
        const haven::application::observability::metrics::MetricLabels&) override {}
};

TEST(CreateReservationHandlerTest,
     Handle_ShouldCreateConfirmedReservation_WhenResourceDoesNotRequireApproval) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    auto idempotency_repository = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto creation_policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resource_repository.add(make_resource(resource_id, organization_id, false));
    const auto handler = CreateReservationHandler{resource_repository,
                                                  reservation_repository,
                                                  creation_store,
                                                  event_store,
                                                  idempotency_repository,
                                                  creation_policy,
                                                  metrics_recorder};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    ASSERT_EQ(result.status(), CreateReservationStatus::CREATED_CONFIRMED);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->status(), ReservationStatus::Confirmed);
    ASSERT_TRUE(creation_store.persisted_reservation().has_value());
    EXPECT_EQ(creation_store.persisted_organization_id(), organization_id);
    ASSERT_EQ(creation_store.persisted_domain_events().size(), 2U);
    EXPECT_TRUE(std::holds_alternative<haven::domain::ReservationCreatedEvent>(
        creation_store.persisted_domain_events()[0]));
    EXPECT_TRUE(std::holds_alternative<haven::domain::ReservationConfirmedEvent>(
        creation_store.persisted_domain_events()[1]));
    EXPECT_EQ(reservation_repository.insert_call_count(), 0U);
    expect_metrics(metrics_recorder, {"created_confirmed"});
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldCreatePendingReservation_WhenResourceRequiresApproval) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-executive-room"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    auto idempotency_repository = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto creation_policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resource_repository.add(make_resource(resource_id, organization_id, true));
    const auto handler = CreateReservationHandler{resource_repository,
                                                  reservation_repository,
                                                  creation_store,
                                                  event_store,
                                                  idempotency_repository,
                                                  creation_policy,
                                                  metrics_recorder};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    ASSERT_EQ(result.status(), CreateReservationStatus::CREATED_PENDING_APPROVAL);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->status(), ReservationStatus::PendingApproval);
    EXPECT_TRUE(creation_store.persisted_reservation().has_value());
    ASSERT_EQ(creation_store.persisted_domain_events().size(), 2U);
    EXPECT_TRUE(std::holds_alternative<haven::domain::ReservationCreatedEvent>(
        creation_store.persisted_domain_events()[0]));
    EXPECT_TRUE(std::holds_alternative<haven::domain::ReservationApprovalRequestedEvent>(
        creation_store.persisted_domain_events()[1]));
    expect_metrics(metrics_recorder, {"created_pending_approval"});
}

TEST(CreateReservationHandlerTest, Handle_ShouldReturnNotFound_WhenResourceDoesNotExist) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-missing"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    auto idempotency_repository = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto creation_policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto handler = CreateReservationHandler{resource_repository,
                                                  reservation_repository,
                                                  creation_store,
                                                  event_store,
                                                  idempotency_repository,
                                                  creation_policy,
                                                  metrics_recorder};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    EXPECT_EQ(result.status(), CreateReservationStatus::RESOURCE_NOT_FOUND);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(creation_store.persisted_reservation().has_value());
    expect_metrics(metrics_recorder, {"resource_not_found"});
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldReturnNotFound_WhenResourceBelongsToAnotherOrganization) {
    const auto caller_organization_id = OrganizationId{"organization-alpha"};
    const auto owner_organization_id = OrganizationId{"organization-beta"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    auto idempotency_repository = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto creation_policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resource_repository.add(make_resource(resource_id, owner_organization_id, false));
    const auto handler = CreateReservationHandler{resource_repository,
                                                  reservation_repository,
                                                  creation_store,
                                                  event_store,
                                                  idempotency_repository,
                                                  creation_policy,
                                                  metrics_recorder};

    const auto result = handler.handle(make_command(caller_organization_id, resource_id));

    EXPECT_EQ(result.status(), CreateReservationStatus::RESOURCE_NOT_FOUND);
    EXPECT_FALSE(creation_store.persisted_reservation().has_value());
    expect_metrics(metrics_recorder, {"resource_not_found"});
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldReturnConflict_WhenConfirmedReservationOverlapsInterval) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    auto idempotency_repository = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto creation_policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resource_repository.add(make_resource(resource_id, organization_id, false));
    reservation_repository.set_conflict(true);
    const auto handler = CreateReservationHandler{resource_repository,
                                                  reservation_repository,
                                                  creation_store,
                                                  event_store,
                                                  idempotency_repository,
                                                  creation_policy,
                                                  metrics_recorder};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    EXPECT_EQ(result.status(), CreateReservationStatus::SCHEDULE_CONFLICT);
    EXPECT_FALSE(creation_store.persisted_reservation().has_value());
    expect_metrics(metrics_recorder, {"reservation_conflict"});
}

TEST(CreateReservationHandlerTest, InactiveResourceRecordsBoundedOutcome) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-inactive"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder = RecordingMetricsRecorder{};
    resources.add(Resource{organization,
                           resource,
                           ResourceType::MeetingRoom,
                           haven::domain::ResourceStatus::Inactive,
                           false});
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_EQ(handler.handle(make_command(organization, resource)).status(),
              CreateReservationStatus::RESOURCE_INACTIVE);
    expect_metrics(metrics_recorder, {"resource_inactive"});
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldRejectStandardReservation_WhenDurationExceedsTwelveHours) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    auto idempotency_repository = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto creation_policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resource_repository.add(make_resource(resource_id, organization_id, false));
    const auto handler = CreateReservationHandler{resource_repository,
                                                  reservation_repository,
                                                  creation_store,
                                                  event_store,
                                                  idempotency_repository,
                                                  creation_policy,
                                                  metrics_recorder};
    const auto command =
        CreateReservationCommand{organization_id,
                                 IdempotencyKey{"idempotency-key-100"},
                                 ReservationId{"reservation-100"},
                                 resource_id,
                                 UserId{"user-100"},
                                 TimeInterval{make_time_point(10), make_time_point(23)},
                                 Purpose{"Planning meeting"},
                                 ReservationKind::Standard,
                                 false,
                                 EventId{"event-created-100"},
                                 EventId{"event-confirmed-100"},
                                 EventId{"event-approval-100"},
                                 make_time_point(9)};

    const auto result = handler.handle(command);

    EXPECT_EQ(result.status(), CreateReservationStatus::POLICY_REJECTED);
    EXPECT_FALSE(creation_store.persisted_reservation().has_value());
    expect_metrics(metrics_recorder, {"validation_failed"});
}

TEST(CreateReservationHandlerTest, SuccessfulReplaySkipsAuthoritativeRepositories) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resources.add(make_resource(resource, organization, false));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};
    const auto command = make_command(organization, resource);

    const auto original = handler.handle(command);
    const auto replay_command =
        CreateReservationCommand{organization,
                                 command.idempotency_key(),
                                 ReservationId{"new-reservation"},
                                 resource,
                                 command.creator_id(),
                                 command.interval(),
                                 command.purpose(),
                                 command.reservation_kind(),
                                 command.maintenance_authorized(),
                                 EventId{"new-created"},
                                 EventId{"new-confirmed"},
                                 EventId{"new-approval"},
                                 command.occurred_at() + std::chrono::hours{1}};
    const auto replayed = handler.handle(replay_command);

    EXPECT_EQ(original.status(), CreateReservationStatus::CREATED_CONFIRMED);
    EXPECT_EQ(replayed.status(), CreateReservationStatus::CREATED_CONFIRMED);
    ASSERT_TRUE(replayed.reservation());
    EXPECT_EQ(replayed.reservation()->reservation_id(), command.reservation_id());
    EXPECT_EQ(resources.find_call_count(), 1);
    EXPECT_EQ(reservations.conflict_call_count(), 1);
    EXPECT_EQ(creation_store.persist_call_count(), 1);
    EXPECT_EQ(reservations.find_call_count(), 0);
    EXPECT_EQ(idempotency.successful_completion_call_count(), 1);
    EXPECT_EQ(event_store.call_count(), 0U);
    expect_metrics(metrics_recorder, {"created_confirmed", "replayed"});
}

TEST(CreateReservationHandlerTest, ExistingProcessingReturnsInProgressWithoutCreation) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto command = make_command(organization, resource);
    static_cast<void>(idempotency.claim(processing_for(command)));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    const auto result = handler.handle(command);

    EXPECT_EQ(result.status(), CreateReservationStatus::IDEMPOTENCY_IN_PROGRESS);
    EXPECT_EQ(resources.find_call_count(), 0);
    EXPECT_EQ(reservations.conflict_call_count(), 0);
    EXPECT_EQ(creation_store.persist_call_count(), 0);
    EXPECT_EQ(idempotency.successful_completion_call_count(), 0);
    expect_metrics(metrics_recorder, {"idempotency_in_progress"});
}

TEST(CreateReservationHandlerTest, CreationStoreFailurePropagatesWithoutIdempotencyCompletion) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resources.add(make_resource(resource, organization, false));
    creation_store.force_failure();
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_THROW(static_cast<void>(handler.handle(make_command(organization, resource))),
                 haven::application::RepositoryError);
    EXPECT_EQ(creation_store.persist_call_count(), 1U);
    EXPECT_EQ(idempotency.successful_completion_call_count(), 0U);
    EXPECT_EQ(idempotency.permanent_failure_completion_call_count(), 0U);
    expect_metrics(metrics_recorder, {"persistence_failed"});
}

TEST(CreateReservationHandlerTest, RecoveryWithoutExpectedOutboxEventsRemainsInProgress) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto command = make_command(organization, resource);
    static_cast<void>(idempotency.claim(processing_for(command)));
    reservations.store(Reservation::create_confirmed(organization,
                                                     command.reservation_id(),
                                                     resource,
                                                     command.creator_id(),
                                                     command.interval(),
                                                     command.purpose(),
                                                     command.reservation_kind(),
                                                     command.created_event_id(),
                                                     command.confirmed_event_id(),
                                                     command.occurred_at()));
    event_store.set_result(false);
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_EQ(handler.handle(command).status(), CreateReservationStatus::IDEMPOTENCY_IN_PROGRESS);
    EXPECT_EQ(event_store.event_ids(),
              (std::vector<EventId>{command.created_event_id(), command.confirmed_event_id()}));
    EXPECT_EQ(creation_store.persist_call_count(), 0U);
    EXPECT_EQ(idempotency.successful_completion_call_count(), 0U);
}

TEST(CreateReservationHandlerTest, FingerprintMismatchReturnsConflictWithoutCreation) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto command = make_command(organization, resource);
    auto different = CreateReservationCommand{organization,
                                              command.idempotency_key(),
                                              command.reservation_id(),
                                              resource,
                                              command.creator_id(),
                                              command.interval(),
                                              Purpose{" Planning meeting"},
                                              command.reservation_kind(),
                                              command.maintenance_authorized(),
                                              command.created_event_id(),
                                              command.confirmed_event_id(),
                                              command.approval_requested_event_id(),
                                              command.occurred_at()};
    static_cast<void>(idempotency.claim(processing_for(different)));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    const auto result = handler.handle(command);

    EXPECT_EQ(result.status(), CreateReservationStatus::IDEMPOTENCY_CONFLICT);
    EXPECT_EQ(resources.find_call_count(), 0);
    EXPECT_EQ(creation_store.persist_call_count(), 0);
    EXPECT_EQ(event_store.call_count(), 0U);
    expect_metrics(metrics_recorder, {"idempotency_mismatch"});
}

TEST(CreateReservationHandlerTest, RejectionReplayRemainsPermanentAfterConditionsChange) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};
    const auto command = make_command(organization, resource);
    EXPECT_EQ(handler.handle(command).status(), CreateReservationStatus::RESOURCE_NOT_FOUND);
    resources.add(make_resource(resource, organization, false));

    const auto replayed = handler.handle(command);

    EXPECT_EQ(replayed.status(), CreateReservationStatus::RESOURCE_NOT_FOUND);
    EXPECT_EQ(resources.find_call_count(), 1);
    EXPECT_EQ(creation_store.persist_call_count(), 0);
    EXPECT_EQ(idempotency.permanent_failure_completion_call_count(), 1);
}

TEST(CreateReservationHandlerTest, CompletionFailureRecoversOnNextInvocation) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resources.add(make_resource(resource, organization, false));
    idempotency.force_successful_completion_failure();
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};
    const auto command = make_command(organization, resource);

    EXPECT_THROW(static_cast<void>(handler.handle(command)), haven::application::RepositoryError);
    EXPECT_EQ(creation_store.persist_call_count(), 1);
    ASSERT_TRUE(creation_store.persisted_reservation());
    reservations.store(*creation_store.persisted_reservation());
    ASSERT_TRUE(idempotency.find(processing_for(command).scope()));
    EXPECT_EQ(idempotency.find(processing_for(command).scope())->status(),
              haven::application::idempotency::IdempotencyStatus::Processing);

    const auto recovered = handler.handle(command);

    EXPECT_EQ(recovered.status(), CreateReservationStatus::CREATED_CONFIRMED);
    EXPECT_EQ(resources.find_call_count(), 1);
    EXPECT_EQ(reservations.conflict_call_count(), 1);
    EXPECT_EQ(creation_store.persist_call_count(), 1);
    EXPECT_EQ(reservations.find_call_count(), 1);
    EXPECT_EQ(event_store.event_ids(),
              (std::vector<EventId>{command.created_event_id(), command.confirmed_event_id()}));
    EXPECT_EQ(idempotency.find(processing_for(command).scope())->status(),
              haven::application::idempotency::IdempotencyStatus::Succeeded);
}

TEST(CreateReservationHandlerTest, RecoveryRejectsExactPurposeMismatchWithoutInsert) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto command = make_command(organization, resource);
    static_cast<void>(idempotency.claim(processing_for(command)));
    reservations.store(Reservation::create_confirmed(organization,
                                                     command.reservation_id(),
                                                     resource,
                                                     command.creator_id(),
                                                     command.interval(),
                                                     Purpose{" Planning meeting"},
                                                     command.reservation_kind(),
                                                     command.created_event_id(),
                                                     command.confirmed_event_id(),
                                                     command.occurred_at()));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_EQ(handler.handle(command).status(), CreateReservationStatus::IDEMPOTENCY_CONFLICT);
    EXPECT_EQ(resources.find_call_count(), 0);
    EXPECT_EQ(creation_store.persist_call_count(), 0);
    EXPECT_EQ(idempotency.successful_completion_call_count(), 0);
}

TEST(CreateReservationHandlerTest, PendingCompletionFailureRecoversPendingResult) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    resources.add(make_resource(resource, organization, true));
    idempotency.force_successful_completion_failure();
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};
    const auto command = make_command(organization, resource);
    EXPECT_THROW(static_cast<void>(handler.handle(command)), haven::application::RepositoryError);
    ASSERT_TRUE(creation_store.persisted_reservation());
    reservations.store(*creation_store.persisted_reservation());

    const auto recovered = handler.handle(command);

    EXPECT_EQ(recovered.status(), CreateReservationStatus::CREATED_PENDING_APPROVAL);
    EXPECT_EQ(creation_store.persist_call_count(), 1);
    EXPECT_EQ(resources.find_call_count(), 1);
    EXPECT_EQ(
        event_store.event_ids(),
        (std::vector<EventId>{command.created_event_id(), command.approval_requested_event_id()}));
}

TEST(CreateReservationHandlerTest, RecoveryRejectsReservationApprovedAfterPendingCreation) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder =
        haven::test::application::observability::metrics::RecordingMetricsRecorder{};
    const auto command = make_command(organization, resource);
    static_cast<void>(idempotency.claim(processing_for(command)));
    auto mutated = Reservation::create_pending_approval(organization,
                                                        command.reservation_id(),
                                                        resource,
                                                        command.creator_id(),
                                                        command.interval(),
                                                        command.purpose(),
                                                        command.reservation_kind(),
                                                        command.created_event_id(),
                                                        command.approval_requested_event_id(),
                                                        command.occurred_at());
    mutated.approve(UserId{"approver"}, command.occurred_at() + 1h, EventId{"approved"});
    reservations.store(std::move(mutated));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_EQ(handler.handle(command).status(), CreateReservationStatus::IDEMPOTENCY_CONFLICT);
    EXPECT_EQ(creation_store.persist_call_count(), 0);
}

TEST(CreateReservationHandlerTest, MetricsFailuresDoNotChangeSuccessfulBusinessResult) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder = ThrowingMetricsRecorder{};
    resources.add(make_resource(resource, organization, false));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_EQ(handler.handle(make_command(organization, resource)).status(),
              CreateReservationStatus::CREATED_CONFIRMED);
    EXPECT_EQ(metrics_recorder.counter_calls, 1U);
    EXPECT_EQ(metrics_recorder.duration_calls, 1U);
    EXPECT_EQ(creation_store.persist_call_count(), 1U);
}

TEST(CreateReservationHandlerTest, MetricsFailuresDoNotChangeExistingBusinessRejection) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-missing"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder = ThrowingMetricsRecorder{};
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_EQ(handler.handle(make_command(organization, resource)).status(),
              CreateReservationStatus::RESOURCE_NOT_FOUND);
    EXPECT_EQ(metrics_recorder.counter_calls, 1U);
    EXPECT_EQ(metrics_recorder.duration_calls, 1U);
    EXPECT_EQ(creation_store.persist_call_count(), 0U);
}

TEST(CreateReservationHandlerTest, UnexpectedExceptionIsRecordedAndPropagatedUnchanged) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder = RecordingMetricsRecorder{};
    resources.force_unexpected_failure();
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};

    EXPECT_THROW(static_cast<void>(handler.handle(make_command(organization, resource))),
                 std::runtime_error);
    expect_metrics(metrics_recorder, {"unexpected_failure"});
}

TEST(CreateReservationHandlerTest, ConcurrentRecoveryReturnsSuccessWithoutAnotherInsert) {
    const auto organization = OrganizationId{"organization-alpha"};
    const auto resource = ResourceId{"resource-boardroom"};
    auto resources = InMemoryResourceRepository{};
    auto reservations = InMemoryReservationRepository{};
    auto idempotency = TestIdempotencyRepository{};
    auto creation_store = haven::tests::util::application::TestReservationCreationStore{};
    auto event_store = haven::tests::util::application::TestReservationCreationEventStore{};
    const auto policy = ReservationCreationPolicy{};
    auto metrics_recorder = DiscardingMetricsRecorder{};
    const auto command = make_command(organization, resource);
    static_cast<void>(idempotency.claim(processing_for(command)));
    reservations.store(Reservation::create_confirmed(organization,
                                                     command.reservation_id(),
                                                     resource,
                                                     command.creator_id(),
                                                     command.interval(),
                                                     command.purpose(),
                                                     command.reservation_kind(),
                                                     command.created_event_id(),
                                                     command.confirmed_event_id(),
                                                     command.occurred_at()));
    const auto handler = CreateReservationHandler{resources,
                                                  reservations,
                                                  creation_store,
                                                  event_store,
                                                  idempotency,
                                                  policy,
                                                  metrics_recorder};
    std::vector<std::future<CreateReservationStatus>> calls;
    for (int index = 0; index < 8; ++index) {
        calls.push_back(std::async(
            std::launch::async, [&handler, &command] { return handler.handle(command).status(); }));
    }
    for (auto& call : calls) {
        EXPECT_EQ(call.get(), CreateReservationStatus::CREATED_CONFIRMED);
    }
    EXPECT_EQ(creation_store.persist_call_count(), 0);
    EXPECT_EQ(idempotency.find(processing_for(command).scope())->status(),
              haven::application::idempotency::IdempotencyStatus::Succeeded);
}

}  // namespace
}  // namespace haven::application::reservations
