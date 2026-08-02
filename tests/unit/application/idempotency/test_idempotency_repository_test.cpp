/**
 * @file test_idempotency_repository_test.cpp
 * @brief Tests the application idempotency repository contract using its in-memory adapter.
 */

#include "application/idempotency/test_idempotency_repository.hpp"

#include <gtest/gtest.h>

#include <barrier>
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace haven::test::application::idempotency {
namespace {

namespace app = haven::application::idempotency;
using haven::application::reservations::CreateReservationStatus;
using haven::domain::EventId;
using haven::domain::IdempotencyKey;
using haven::domain::OrganizationId;
using haven::domain::ReservationId;
using haven::domain::ReservationKind;
using haven::domain::ReservationStatus;
using haven::domain::ResourceId;
using haven::domain::UserId;
using haven::domain::Version;

[[nodiscard]] auto created_at() {
    return std::chrono::system_clock::time_point{std::chrono::hours{9}};
}

[[nodiscard]] app::IdempotencyScope scope(
    std::string organization = "organization-1",
    std::string creator = "user-1",
    const app::IdempotencyOperation operation = app::IdempotencyOperation::CreateReservation) {
    return app::IdempotencyScope{OrganizationId{std::move(organization)},
                                 UserId{std::move(creator)},
                                 operation,
                                 IdempotencyKey{"same-key"}};
}

[[nodiscard]] app::ReservationCreationIdentifiers identifiers() {
    return app::ReservationCreationIdentifiers{ReservationId{"reservation-1"},
                                               EventId{"event-created"},
                                               EventId{"event-confirmed"},
                                               EventId{"event-approval"}};
}

[[nodiscard]] app::IdempotencyRecord processing(app::IdempotencyScope value = scope(),
                                                std::string fingerprint = "fingerprint-1") {
    return app::IdempotencyRecord::processing(std::move(value),
                                              app::IdempotencyFingerprint{std::move(fingerprint)},
                                              identifiers(),
                                              created_at());
}

[[nodiscard]] app::CreateReservationResultSnapshot success(std::string resource = "resource-1") {
    return app::CreateReservationResultSnapshot::successful(
        CreateReservationStatus::CREATED_CONFIRMED,
        ReservationId{"reservation-1"},
        ResourceId{std::move(resource)},
        ReservationStatus::Confirmed,
        ReservationKind::Standard,
        Version{1},
        created_at());
}

[[nodiscard]] app::CreateReservationResultSnapshot rejection(
    const CreateReservationStatus status = CreateReservationStatus::SCHEDULE_CONFLICT) {
    return app::CreateReservationResultSnapshot::permanent_rejection(status);
}

void expect_error(const app::IdempotencyRepositoryErrorCode expected,
                  const std::function<void()>& operation) {
    try {
        operation();
        FAIL() << "Expected IdempotencyRepositoryError";
    } catch (const app::IdempotencyRepositoryError& error) {
        EXPECT_EQ(error.code(), expected);
    }
}

TEST(TestIdempotencyRepositoryClaimTest, FirstClaimPersistsProcessingRecord) {
    auto repository = TestIdempotencyRepository{};
    const auto result = repository.claim(processing());
    EXPECT_EQ(result.status(), app::IdempotencyClaimStatus::Claimed);
    EXPECT_EQ(result.record().status(), app::IdempotencyStatus::Processing);
    EXPECT_EQ(repository.find(scope())->status(), app::IdempotencyStatus::Processing);
    EXPECT_EQ(repository.stored_record_count(), 1U);
}

TEST(TestIdempotencyRepositoryClaimTest, FindReturnsEmptyWhenScopeIsAbsent) {
    const auto repository = TestIdempotencyRepository{};
    EXPECT_FALSE(repository.find(scope()).has_value());
}

TEST(TestIdempotencyRepositoryClaimTest, IdenticalSecondClaimReturnsExistingProcessing) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    const auto result = repository.claim(processing());
    EXPECT_EQ(result.status(), app::IdempotencyClaimStatus::ExistingProcessing);
    EXPECT_EQ(result.record().fingerprint(), app::IdempotencyFingerprint{"fingerprint-1"});
    EXPECT_EQ(repository.stored_record_count(), 1U);
}

TEST(TestIdempotencyRepositoryClaimTest, DifferentFingerprintReturnsMismatchWithStoredRecord) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    const auto result = repository.claim(processing(scope(), "fingerprint-2"));
    EXPECT_EQ(result.status(), app::IdempotencyClaimStatus::FingerprintMismatch);
    EXPECT_EQ(result.record().fingerprint(), app::IdempotencyFingerprint{"fingerprint-1"});
}

TEST(TestIdempotencyRepositoryClaimTest, CompleteScopeSeparatesClaims) {
    auto repository = TestIdempotencyRepository{};
    EXPECT_EQ(repository.claim(processing()).status(), app::IdempotencyClaimStatus::Claimed);
    EXPECT_EQ(repository.claim(processing(scope("organization-2"))).status(),
              app::IdempotencyClaimStatus::Claimed);
    EXPECT_EQ(repository.claim(processing(scope("organization-1", "user-2"))).status(),
              app::IdempotencyClaimStatus::Claimed);
    EXPECT_EQ(repository
                  .claim(processing(scope(
                      "organization-1", "user-1", app::IdempotencyOperation::ExtendReservation)))
                  .status(),
              app::IdempotencyClaimStatus::Claimed);
    EXPECT_EQ(repository.stored_record_count(), 4U);
}

TEST(TestIdempotencyRepositoryCompletionTest, SuccessPreservesFixedRecordIdentity) {
    auto repository = TestIdempotencyRepository{};
    const auto original = repository.claim(processing()).record();
    repository.record_succeeded(scope(), original.fingerprint(), success());
    const auto stored = *repository.find(scope());
    EXPECT_EQ(stored.status(), app::IdempotencyStatus::Succeeded);
    EXPECT_EQ(stored.result(), success());
    EXPECT_EQ(stored.scope(), original.scope());
    EXPECT_EQ(stored.fingerprint(), original.fingerprint());
    EXPECT_EQ(stored.generated_identifiers(), original.generated_identifiers());
    EXPECT_EQ(stored.created_at(), original.created_at());
}

TEST(TestIdempotencyRepositoryCompletionTest, EquivalentRepeatedSuccessIsIdempotent) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    repository.record_succeeded(scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success());
    EXPECT_NO_THROW(repository.record_succeeded(
        scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success()));
    EXPECT_EQ(repository.successful_completion_call_count(), 2U);
}

TEST(TestIdempotencyRepositoryCompletionTest, SuccessCannotBeOverwritten) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    repository.record_succeeded(scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success());
    expect_error(app::IdempotencyRepositoryErrorCode::TerminalConflict, [&] {
        repository.record_succeeded(
            scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success("resource-2"));
    });
    expect_error(app::IdempotencyRepositoryErrorCode::TerminalConflict, [&] {
        repository.record_failed_permanently(
            scope(), app::IdempotencyFingerprint{"fingerprint-1"}, rejection());
    });
}

TEST(TestIdempotencyRepositoryCompletionTest, PermanentFailurePreservesSnapshot) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    repository.record_failed_permanently(
        scope(), app::IdempotencyFingerprint{"fingerprint-1"}, rejection());
    const auto stored = *repository.find(scope());
    EXPECT_EQ(stored.status(), app::IdempotencyStatus::FailedPermanent);
    EXPECT_EQ(stored.result(), rejection());
    const auto replay = repository.claim(processing());
    EXPECT_EQ(replay.status(), app::IdempotencyClaimStatus::ExistingFailedPermanent);
    EXPECT_EQ(replay.record().result(), rejection());
}

TEST(TestIdempotencyRepositoryCompletionTest, EquivalentRepeatedFailureIsIdempotent) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    repository.record_failed_permanently(
        scope(), app::IdempotencyFingerprint{"fingerprint-1"}, rejection());
    EXPECT_NO_THROW(repository.record_failed_permanently(
        scope(), app::IdempotencyFingerprint{"fingerprint-1"}, rejection()));
    EXPECT_EQ(repository.permanent_failure_completion_call_count(), 2U);
}

TEST(TestIdempotencyRepositoryCompletionTest, PermanentFailureCannotBeOverwritten) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    repository.record_failed_permanently(
        scope(), app::IdempotencyFingerprint{"fingerprint-1"}, rejection());
    expect_error(app::IdempotencyRepositoryErrorCode::TerminalConflict, [&] {
        repository.record_failed_permanently(scope(),
                                             app::IdempotencyFingerprint{"fingerprint-1"},
                                             rejection(CreateReservationStatus::RESOURCE_INACTIVE));
    });
    expect_error(app::IdempotencyRepositoryErrorCode::TerminalConflict, [&] {
        repository.record_succeeded(
            scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success());
    });
}

TEST(TestIdempotencyRepositoryValidationTest, ClaimRejectsTerminalRecord) {
    auto repository = TestIdempotencyRepository{};
    const auto terminal =
        app::IdempotencyRecord::succeeded(scope(),
                                          app::IdempotencyFingerprint{"fingerprint-1"},
                                          identifiers(),
                                          created_at(),
                                          success());
    expect_error(app::IdempotencyRepositoryErrorCode::InvalidRecord,
                 [&] { static_cast<void>(repository.claim(terminal)); });
}

TEST(TestIdempotencyRepositoryValidationTest, CompletionRejectsWrongSnapshotKind) {
    auto repository = TestIdempotencyRepository{};
    expect_error(app::IdempotencyRepositoryErrorCode::InvalidSnapshot, [&] {
        repository.record_succeeded(
            scope(), app::IdempotencyFingerprint{"fingerprint-1"}, rejection());
    });
    expect_error(app::IdempotencyRepositoryErrorCode::InvalidSnapshot, [&] {
        repository.record_failed_permanently(
            scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success());
    });
}

TEST(TestIdempotencyRepositoryValidationTest, CompletionRejectsMissingRecord) {
    auto repository = TestIdempotencyRepository{};
    expect_error(app::IdempotencyRepositoryErrorCode::MissingRecord, [&] {
        repository.record_succeeded(
            scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success());
    });
}

TEST(TestIdempotencyRepositoryValidationTest, CompletionRejectsWrongFingerprint) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    expect_error(app::IdempotencyRepositoryErrorCode::FingerprintMismatch, [&] {
        repository.record_succeeded(
            scope(), app::IdempotencyFingerprint{"fingerprint-2"}, success());
    });
}

TEST(IdempotencyClaimResultTest, FactoriesRequireCompatibleRecordStates) {
    const auto processing_record = processing();
    const auto succeeded_record =
        app::IdempotencyRecord::succeeded(scope(),
                                          app::IdempotencyFingerprint{"fingerprint-1"},
                                          identifiers(),
                                          created_at(),
                                          success());
    const auto failed_record =
        app::IdempotencyRecord::failed_permanently(scope(),
                                                   app::IdempotencyFingerprint{"fingerprint-1"},
                                                   identifiers(),
                                                   created_at(),
                                                   rejection());
    EXPECT_EQ(app::IdempotencyClaimResult::claimed(processing_record).record().status(),
              app::IdempotencyStatus::Processing);
    EXPECT_EQ(app::IdempotencyClaimResult::existing_processing(processing_record).record().status(),
              app::IdempotencyStatus::Processing);
    EXPECT_EQ(app::IdempotencyClaimResult::existing_succeeded(succeeded_record).record().status(),
              app::IdempotencyStatus::Succeeded);
    EXPECT_EQ(
        app::IdempotencyClaimResult::existing_failed_permanently(failed_record).record().status(),
        app::IdempotencyStatus::FailedPermanent);
    EXPECT_EQ(app::IdempotencyClaimResult::fingerprint_mismatch(failed_record).status(),
              app::IdempotencyClaimStatus::FingerprintMismatch);
    EXPECT_THROW(
        static_cast<void>(app::IdempotencyClaimResult::existing_processing(succeeded_record)),
        std::invalid_argument);
}

TEST(IdempotencyClaimResultTest, ExistingTerminalClaimsReturnStoredResult) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    repository.record_succeeded(scope(), app::IdempotencyFingerprint{"fingerprint-1"}, success());
    const auto result = repository.claim(processing());
    EXPECT_EQ(result.status(), app::IdempotencyClaimStatus::ExistingSucceeded);
    EXPECT_EQ(result.record().result(), success());
}

TEST(TestIdempotencyRepositoryConcurrencyTest, SameFingerprintHasExactlyOneClaimWinner) {
    constexpr std::size_t thread_count{12};
    auto repository = TestIdempotencyRepository{};
    auto start = std::barrier{static_cast<std::ptrdiff_t>(thread_count)};
    auto outcomes = std::vector<app::IdempotencyClaimStatus>(thread_count);
    auto threads = std::vector<std::thread>{};
    for (std::size_t index = 0; index < thread_count; ++index) {
        threads.emplace_back([&, index] {
            start.arrive_and_wait();
            outcomes[index] = repository.claim(processing()).status();
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(std::count(outcomes.begin(), outcomes.end(), app::IdempotencyClaimStatus::Claimed),
              1);
    EXPECT_EQ(
        std::count(
            outcomes.begin(), outcomes.end(), app::IdempotencyClaimStatus::ExistingProcessing),
        thread_count - 1);
    EXPECT_EQ(repository.stored_record_count(), 1U);
}

TEST(TestIdempotencyRepositoryConcurrencyTest, DifferentFingerprintsStoreExactlyOneWinner) {
    constexpr std::size_t thread_count{12};
    auto repository = TestIdempotencyRepository{};
    auto start = std::barrier{static_cast<std::ptrdiff_t>(thread_count)};
    auto outcomes = std::vector<app::IdempotencyClaimStatus>(thread_count);
    auto threads = std::vector<std::thread>{};
    for (std::size_t index = 0; index < thread_count; ++index) {
        threads.emplace_back([&, index] {
            start.arrive_and_wait();
            outcomes[index] =
                repository.claim(processing(scope(), "fingerprint-" + std::to_string(index)))
                    .status();
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(std::count(outcomes.begin(), outcomes.end(), app::IdempotencyClaimStatus::Claimed),
              1);
    EXPECT_EQ(
        std::count(
            outcomes.begin(), outcomes.end(), app::IdempotencyClaimStatus::FingerprintMismatch),
        thread_count - 1);
    EXPECT_EQ(repository.stored_record_count(), 1U);
}

TEST(TestIdempotencyRepositoryObservationTest, CountsCallsAndRecordsLastScope) {
    auto repository = TestIdempotencyRepository{};
    static_cast<void>(repository.claim(processing()));
    static_cast<void>(repository.find(scope()));
    EXPECT_EQ(repository.claim_call_count(), 1U);
    EXPECT_EQ(repository.find_call_count(), 1U);
    EXPECT_EQ(repository.last_claimed_scope(), scope());
}

TEST(TestIdempotencyRepositoryObservationTest, ForcedFailureUsesGenericRepositoryError) {
    auto repository = TestIdempotencyRepository{};
    repository.force_repository_failure();
    EXPECT_THROW(static_cast<void>(repository.claim(processing())),
                 haven::application::RepositoryError);
}

}  // namespace
}  // namespace haven::test::application::idempotency
