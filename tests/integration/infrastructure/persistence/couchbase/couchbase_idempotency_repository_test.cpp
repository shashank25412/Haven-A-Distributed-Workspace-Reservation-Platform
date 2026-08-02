#include "haven/infrastructure/persistence/couchbase/couchbase_idempotency_repository.hpp"

#include "haven/application/idempotency/idempotency_repository_error.hpp"
#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
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
            connection_, std::chrono::seconds{30});
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
            record.generated_identifiers().reservation_id,
            haven::domain::ResourceId{resource},
            haven::domain::ReservationStatus::Confirmed,
            haven::domain::ReservationKind::Standard,
            haven::domain::Version{1},
            record.created_at());
    }

    std::optional<persistence::CouchbaseConfiguration> config_;
    std::shared_ptr<persistence::CouchbaseConnection> connection_;
    std::unique_ptr<persistence::CouchbaseIdempotencyRepository> repository_;
    std::vector<std::string> keys_;
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
    persistence::CouchbaseIdempotencyRepository restarted(connection_, std::chrono::seconds{30});
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
            persistence::CouchbaseIdempotencyRepository repository(connection_,
                                                                   std::chrono::seconds{30});
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
    persistence::CouchbaseIdempotencyRepository short_lived(connection_, std::chrono::seconds{2});
    short_lived.claim(initial);
    std::this_thread::sleep_for(std::chrono::milliseconds{800});
    short_lived.record_succeeded(initial.scope(), initial.fingerprint(), success(initial));
    // Couchbase expiry removal is second-granularity and may become observable
    // shortly after the exact deadline.
    std::this_thread::sleep_for(std::chrono::milliseconds{2700});
    EXPECT_FALSE(short_lived.find(initial.scope()).has_value());
}

}  // namespace
