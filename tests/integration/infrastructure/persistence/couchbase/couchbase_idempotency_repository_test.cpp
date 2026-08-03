/**
 * @file couchbase_idempotency_repository_test.cpp
 * @brief Tests durable idempotency behavior against Couchbase.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_idempotency_repository.hpp"

#include "haven/application/idempotency/create_reservation_fingerprint_input.hpp"
#include "haven/application/idempotency/idempotency_repository_error.hpp"
#include "haven/application/reservations/create_reservation_handler.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/policies/reservation_creation_policy.hpp"
#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_event_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_key.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <couchbase/error_codes.hxx>
#include <cstdlib>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {
namespace persistence = haven::infrastructure::persistence::couchbase;
using namespace haven::application::idempotency;
using namespace haven::application::reservations;

class RecoveryResourceRepository final : public haven::application::resources::ResourceRepository {
public:
    haven::application::resources::ResourceLookupResult find_by_id(
        const haven::domain::OrganizationId&, const haven::domain::ResourceId&) const override {
        ++calls;
        return std::nullopt;
    }
    haven::application::resources::ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId&, haven::domain::ResourceType) const override {
        return {};
    }
    mutable std::atomic_size_t calls{};
};

std::optional<std::string> env(const char* name) {
    const char* value = std::getenv(name);
    return value && *value ? std::optional<std::string>{value} : std::nullopt;
}

std::optional<persistence::CouchbaseConfiguration> configuration() {
    const auto connection = env("HVN_COUCHBASE_CONNECTION_STRING");
    const auto username = env("HVN_COUCHBASE_USERNAME");
    const auto password = env("HVN_COUCHBASE_PASSWORD");
    const auto bucket = env("HVN_COUCHBASE_BUCKET");
    const auto scope = env("HVN_COUCHBASE_SCOPE");
    if (!connection || !username || !password || !bucket || !scope)
        return std::nullopt;
    return persistence::CouchbaseConfiguration{.connection_string = *connection,
                                               .username = *username,
                                               .password = *password,
                                               .bucket_name = *bucket,
                                               .scope_name = *scope};
}

std::string unique() {
    static std::atomic_uint64_t sequence{};
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
           std::to_string(sequence++);
}

class IdempotencyRepositoryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = configuration();
        if (!config_)
            GTEST_SKIP() << "Set HVN_COUCHBASE_* to run Couchbase integration tests";
        connection_ = std::make_shared<persistence::CouchbaseConnection>(*config_);
        repository_ = std::make_unique<persistence::CouchbaseIdempotencyRepository>(
            connection_, std::chrono::seconds{30}, metrics_);
    }

    void TearDown() override {
        if (!connection_)
            return;
        auto collection = connection_->collection(persistence::CouchbaseCollections::idempotency);
        for (const auto& key : keys_) {
            auto [error, result] = collection.remove(key).get();
            static_cast<void>(result);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << error.ec().message();
        }
        auto reservations =
            connection_->collection(persistence::CouchbaseCollections::reservations);
        for (const auto& key : reservation_keys_) {
            auto [error, result] = reservations.remove(key).get();
            static_cast<void>(result);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << error.ec().message();
        }
        auto outbox = connection_->collection(persistence::CouchbaseCollections::outbox);
        for (const auto& key : outbox_keys_) {
            auto [error, result] = outbox.remove(key).get();
            static_cast<void>(result);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << error.ec().message();
        }
    }

    IdempotencyRecord record(std::string fingerprint = std::string(64, 'a'),
                             std::string organization = {}) {
        const auto suffix = unique();
        auto operation_scope = IdempotencyScope{
            haven::domain::OrganizationId{organization.empty() ? "org-" + suffix : organization},
            haven::domain::UserId{"user-" + suffix},
            IdempotencyOperation::CreateReservation,
            haven::domain::IdempotencyKey{"key-" + suffix}};
        keys_.push_back(persistence::idempotency_document_key(operation_scope));
        return IdempotencyRecord::processing(
            std::move(operation_scope),
            IdempotencyFingerprint{std::move(fingerprint)},
            {haven::domain::ReservationId{"reservation-" + suffix},
             haven::domain::EventId{"created-" + suffix},
             haven::domain::EventId{"confirmed-" + suffix},
             haven::domain::EventId{"approval-" + suffix}},
            IdempotencyRecord::TimePoint{std::chrono::seconds{1'800'000'000}});
    }

    CreateReservationResultSnapshot success(const IdempotencyRecord& record,
                                            std::string resource = "resource") {
        return CreateReservationResultSnapshot::successful(
            CreateReservationStatus::CREATED_CONFIRMED,
            record.scope().organization_id(),
            record.generated_identifiers().reservation_id,
            haven::domain::ResourceId{resource},
            record.scope().creator_id(),
            haven::domain::TimeInterval{record.created_at(),
                                        record.created_at() + std::chrono::hours{1}},
            haven::domain::Purpose{"purpose"},
            haven::domain::ReservationStatus::Confirmed,
            haven::domain::ReservationKind::Standard,
            haven::domain::Version{1},
            record.created_at());
    }

    std::optional<persistence::CouchbaseConfiguration> config_;
    std::shared_ptr<persistence::CouchbaseConnection> connection_;
    haven::infrastructure::observability::metrics::NoOpMetricsRecorder metrics_;
    std::unique_ptr<persistence::CouchbaseIdempotencyRepository> repository_;
    std::vector<std::string> keys_;
    std::vector<std::string> reservation_keys_;
    std::vector<std::string> outbox_keys_;
};

TEST_F(IdempotencyRepositoryIntegrationTest, ClaimsAndClassifiesExistingRecord) {
    const auto initial = record();
    EXPECT_EQ(repository_->claim(initial).status(), IdempotencyClaimStatus::Claimed);
    EXPECT_EQ(repository_->claim(initial).status(), IdempotencyClaimStatus::ExistingProcessing);
    auto mismatch = IdempotencyRecord::processing(initial.scope(),
                                                  IdempotencyFingerprint{std::string(64, 'b')},
                                                  initial.generated_identifiers(),
                                                  initial.created_at());
    EXPECT_EQ(repository_->claim(mismatch).status(), IdempotencyClaimStatus::FingerprintMismatch);
}

TEST_F(IdempotencyRepositoryIntegrationTest, CompletesSuccessAndSurvivesRepositoryReconstruction) {
    const auto initial = record();
    const auto snapshot = success(initial);
    repository_->claim(initial);
    repository_->record_succeeded(initial.scope(), initial.fingerprint(), snapshot);
    repository_->record_succeeded(initial.scope(), initial.fingerprint(), snapshot);
    persistence::CouchbaseIdempotencyRepository restarted(
        connection_, std::chrono::seconds{30}, metrics_);
    const auto found = restarted.find(initial.scope());
    ASSERT_TRUE(found);
    EXPECT_EQ(found->status(), IdempotencyStatus::Succeeded);
    EXPECT_EQ(*found->result(), snapshot);
}

TEST_F(IdempotencyRepositoryIntegrationTest, CompletesPermanentFailureIdempotently) {
    const auto initial = record();
    const auto snapshot = CreateReservationResultSnapshot::permanent_rejection(
        CreateReservationStatus::POLICY_REJECTED);
    repository_->claim(initial);
    repository_->record_failed_permanently(initial.scope(), initial.fingerprint(), snapshot);
    repository_->record_failed_permanently(initial.scope(), initial.fingerprint(), snapshot);
    EXPECT_EQ(repository_->find(initial.scope())->status(), IdempotencyStatus::FailedPermanent);
}

TEST_F(IdempotencyRepositoryIntegrationTest, RejectsConflictingWrongAndMissingCompletions) {
    const auto initial = record();
    repository_->claim(initial);
    repository_->record_succeeded(initial.scope(), initial.fingerprint(), success(initial));
    try {
        repository_->record_succeeded(
            initial.scope(), initial.fingerprint(), success(initial, "other"));
        FAIL();
    } catch (const IdempotencyRepositoryError& error) {
        EXPECT_EQ(error.code(), IdempotencyRepositoryErrorCode::TerminalConflict);
    }
    const auto missing = record();
    try {
        repository_->record_succeeded(missing.scope(), missing.fingerprint(), success(missing));
        FAIL();
    } catch (const IdempotencyRepositoryError& error) {
        EXPECT_EQ(error.code(), IdempotencyRepositoryErrorCode::MissingRecord);
    }
}

TEST_F(IdempotencyRepositoryIntegrationTest, ConcurrentIdenticalClaimsHaveOneWinner) {
    const auto initial = record();
    std::vector<std::future<IdempotencyClaimStatus>> attempts;
    for (int index = 0; index < 8; ++index) {
        attempts.push_back(std::async(std::launch::async, [this, &initial] {
            persistence::CouchbaseIdempotencyRepository repository(
                connection_, std::chrono::seconds{30}, metrics_);
            return repository.claim(initial).status();
        }));
    }
    int claimed{};
    int existing{};
    for (auto& attempt : attempts) {
        const auto status = attempt.get();
        claimed += status == IdempotencyClaimStatus::Claimed;
        existing += status == IdempotencyClaimStatus::ExistingProcessing;
    }
    EXPECT_EQ(claimed, 1);
    EXPECT_EQ(existing, 7);
}

TEST_F(IdempotencyRepositoryIntegrationTest, CompletionPreservesInitialExpiry) {
    const auto initial = record();
    persistence::CouchbaseIdempotencyRepository short_lived(
        connection_, std::chrono::seconds{2}, metrics_);
    short_lived.claim(initial);
    std::this_thread::sleep_for(std::chrono::milliseconds{800});
    short_lived.record_succeeded(initial.scope(), initial.fingerprint(), success(initial));
    // Couchbase expiry removal is second-granularity and may become observable
    // shortly after the exact deadline.
    std::this_thread::sleep_for(std::chrono::milliseconds{2700});
    EXPECT_FALSE(short_lived.find(initial.scope()).has_value());
}

TEST_F(IdempotencyRepositoryIntegrationTest, HandlerRecoversPersistedReservationAfterRestart) {
    const auto suffix = unique();
    const auto organization = haven::domain::OrganizationId{"recovery-org-" + suffix};
    const auto creator = haven::domain::UserId{"recovery-user-" + suffix};
    const auto resource = haven::domain::ResourceId{"recovery-resource-" + suffix};
    const auto reservation_id = haven::domain::ReservationId{"recovery-reservation-" + suffix};
    const auto interval = haven::domain::TimeInterval{
        IdempotencyRecord::TimePoint{std::chrono::seconds{1'800'000'000}},
        IdempotencyRecord::TimePoint{std::chrono::seconds{1'800'003'600}}};
    const auto command =
        CreateReservationCommand{organization,
                                 haven::domain::IdempotencyKey{"recovery-key-" + suffix},
                                 reservation_id,
                                 resource,
                                 creator,
                                 interval,
                                 haven::domain::Purpose{" recovery purpose "},
                                 haven::domain::ReservationKind::Standard,
                                 false,
                                 haven::domain::EventId{"created-" + suffix},
                                 haven::domain::EventId{"confirmed-" + suffix},
                                 haven::domain::EventId{"approval-" + suffix},
                                 interval.start()};
    const auto scope = IdempotencyScope{
        organization, creator, IdempotencyOperation::CreateReservation, command.idempotency_key()};
    const auto fingerprint = create_reservation_fingerprint(CreateReservationFingerprintInput{
        resource, creator, interval, command.purpose(), command.reservation_kind(), false});
    const auto processing = IdempotencyRecord::processing(scope,
                                                          fingerprint,
                                                          {reservation_id,
                                                           command.created_event_id(),
                                                           command.confirmed_event_id(),
                                                           command.approval_requested_event_id()},
                                                          command.occurred_at());
    keys_.push_back(persistence::idempotency_document_key(scope));
    reservation_keys_.push_back(
        persistence::reservation_document_key(organization, reservation_id));
    static_cast<void>(repository_->claim(processing));
    auto reservation = haven::domain::Reservation::create_confirmed(organization,
                                                                    reservation_id,
                                                                    resource,
                                                                    creator,
                                                                    interval,
                                                                    command.purpose(),
                                                                    command.reservation_kind(),
                                                                    command.created_event_id(),
                                                                    command.confirmed_event_id(),
                                                                    command.occurred_at());
    auto events = reservation.release_domain_events();
    for (const auto& event_id : {command.created_event_id(), command.confirmed_event_id()})
        outbox_keys_.push_back(persistence::outbox_document_key(organization, event_id));
    auto creation_store = persistence::CouchbaseReservationCreationStore{connection_, metrics_};
    static_cast<void>(creation_store.persist(organization, reservation, std::move(events)));

    auto fresh_idempotency = persistence::CouchbaseIdempotencyRepository{
        connection_, std::chrono::seconds{30}, metrics_};
    auto fresh_reservations = persistence::CouchbaseReservationRepository{connection_, metrics_};
    auto resources = RecoveryResourceRepository{};
    const auto policy = haven::domain::ReservationCreationPolicy{};
    auto metrics_recorder = haven::infrastructure::observability::metrics::NoOpMetricsRecorder{};
    auto event_store = persistence::CouchbaseReservationCreationEventStore{connection_, metrics_};
    const auto handler = CreateReservationHandler{resources,
                                                  fresh_reservations,
                                                  creation_store,
                                                  event_store,
                                                  fresh_idempotency,
                                                  policy,
                                                  metrics_recorder};
    const auto result = handler.handle(command);

    EXPECT_EQ(result.status(), CreateReservationStatus::CREATED_CONFIRMED);
    EXPECT_EQ(resources.calls.load(), 0);
    EXPECT_EQ(fresh_idempotency.find(scope)->status(), IdempotencyStatus::Succeeded);
    EXPECT_TRUE(fresh_reservations.find_by_id(organization, reservation_id));
}

}  // namespace
