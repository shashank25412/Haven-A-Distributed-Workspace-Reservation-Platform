/**
 * @file idempotency_contract_test.cpp
 * @brief Tests reservation-create idempotency application contracts.
 */

#include "haven/application/idempotency/create_reservation_fingerprint_input.hpp"
#include "haven/application/idempotency/create_reservation_result_snapshot.hpp"
#include "haven/application/idempotency/idempotency_record.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace haven::application::idempotency {
namespace {

using namespace std::chrono_literals;
using haven::application::reservations::CreateReservationStatus;
using haven::domain::EventId;
using haven::domain::IdempotencyKey;
using haven::domain::OrganizationId;
using haven::domain::Purpose;
using haven::domain::ReservationId;
using haven::domain::ReservationKind;
using haven::domain::ReservationStatus;
using haven::domain::ResourceId;
using haven::domain::TimeInterval;
using haven::domain::UserId;
using haven::domain::Version;

static_assert(!std::is_default_constructible_v<IdempotencyFingerprint>);
static_assert(std::is_copy_constructible_v<IdempotencyFingerprint>);
static_assert(std::is_move_constructible_v<IdempotencyFingerprint>);

[[nodiscard]] auto time_point(const int hour) {
    return std::chrono::system_clock::time_point{std::chrono::hours{hour}};
}

[[nodiscard]] CreateReservationFingerprintInput input(
    std::string resource = "resource-1",
    std::string creator = "user-1",
    const int start = 10,
    const int end = 11,
    std::string purpose = "Planning",
    const ReservationKind kind = ReservationKind::Standard,
    const bool maintenance_authorized = false) {
    return CreateReservationFingerprintInput{ResourceId{std::move(resource)},
                                             UserId{std::move(creator)},
                                             TimeInterval{time_point(start), time_point(end)},
                                             Purpose{std::move(purpose)},
                                             kind,
                                             maintenance_authorized};
}

[[nodiscard]] IdempotencyScope scope(
    std::string organization = "organization-1",
    std::string creator = "user-1",
    const IdempotencyOperation operation = IdempotencyOperation::CreateReservation,
    std::string key = "key-1") {
    return IdempotencyScope{OrganizationId{std::move(organization)},
                            UserId{std::move(creator)},
                            operation,
                            IdempotencyKey{std::move(key)}};
}

[[nodiscard]] ReservationCreationIdentifiers identifiers() {
    return ReservationCreationIdentifiers{ReservationId{"reservation-1"},
                                          EventId{"event-created"},
                                          EventId{"event-confirmed"},
                                          EventId{"event-approval"}};
}

[[nodiscard]] CreateReservationResultSnapshot confirmed_snapshot() {
    return CreateReservationResultSnapshot::successful(CreateReservationStatus::CREATED_CONFIRMED,
                                                       ReservationId{"reservation-1"},
                                                       ResourceId{"resource-1"},
                                                       ReservationStatus::Confirmed,
                                                       ReservationKind::Standard,
                                                       Version{1},
                                                       time_point(9));
}

[[nodiscard]] CreateReservationResultSnapshot rejection_snapshot() {
    return CreateReservationResultSnapshot::permanent_rejection(
        CreateReservationStatus::SCHEDULE_CONFLICT);
}

TEST(IdempotencyFingerprintTest, RejectsEmptyEncodedDigest) {
    EXPECT_THROW(IdempotencyFingerprint{""}, std::invalid_argument);
}

TEST(IdempotencyFingerprintTest, IdenticalLogicalRequestsProduceSameNonEmptyDigest) {
    const auto first = create_reservation_fingerprint(input());
    const auto second = create_reservation_fingerprint(input());
    EXPECT_EQ(first, second);
    EXPECT_FALSE(first.value().empty());
    EXPECT_EQ(first.value().size(), 64U);
}

TEST(IdempotencyFingerprintTest, GoldenVectorUsesCanonicalVersionOneEncoding) {
    // Fields, in order: resource-1, user-1, epoch hours 10 and 11,
    // exact purpose "Planning", STANDARD, false. Strings are uint64-length-prefixed.
    EXPECT_EQ(create_reservation_fingerprint(input()).value(),
              "959187f6ca76d288f41426ffe34542e1d374c6742e4aec82587c2f0ce156026b");
}

TEST(IdempotencyFingerprintTest, EveryLogicalPayloadFieldAffectsDigest) {
    const auto original = create_reservation_fingerprint(input());
    EXPECT_NE(original, create_reservation_fingerprint(input("resource-2")));
    EXPECT_NE(original, create_reservation_fingerprint(input("resource-1", "user-2")));
    EXPECT_NE(original, create_reservation_fingerprint(input("resource-1", "user-1", 9)));
    EXPECT_NE(original, create_reservation_fingerprint(input("resource-1", "user-1", 10, 12)));
    EXPECT_NE(original,
              create_reservation_fingerprint(input("resource-1", "user-1", 10, 11, "planning")));
    EXPECT_NE(original,
              create_reservation_fingerprint(
                  input("resource-1", "user-1", 10, 11, "Planning", ReservationKind::Maintenance)));
    EXPECT_NE(original,
              create_reservation_fingerprint(input(
                  "resource-1", "user-1", 10, 11, "Planning", ReservationKind::Standard, true)));
}

TEST(IdempotencyFingerprintTest, PurposeWhitespaceIsPreserved) {
    EXPECT_NE(create_reservation_fingerprint(input()),
              create_reservation_fingerprint(input("resource-1", "user-1", 10, 11, " Planning ")));
}

TEST(IdempotencyFingerprintTest, LengthPrefixesPreventFieldBoundaryAmbiguity) {
    EXPECT_NE(create_reservation_fingerprint(input("a", "bc")),
              create_reservation_fingerprint(input("ab", "c")));
}

TEST(IdempotencyFingerprintTest, ServerGeneratedValuesCannotAffectInput) {
    EXPECT_FALSE((std::is_constructible_v<CreateReservationFingerprintInput,
                                          ReservationId,
                                          EventId,
                                          std::chrono::system_clock::time_point>));
}

TEST(IdempotencyScopeTest, EqualityIncludesEveryScopeField) {
    EXPECT_EQ(scope(), scope());
    EXPECT_NE(scope(), scope("organization-2"));
    EXPECT_NE(scope(), scope("organization-1", "user-2"));
    EXPECT_NE(scope(), scope("organization-1", "user-1", IdempotencyOperation::ExtendReservation));
}

TEST(IdempotencyScopeTest, PreservesRawKeyExactly) {
    const auto value = scope(
        "organization-1", "user-1", IdempotencyOperation::CreateReservation, " Key :: Value ");
    EXPECT_EQ(value.key().value(), " Key :: Value ");
}

TEST(IdempotencyRecordTest, ProcessingHasNoTerminalSnapshot) {
    const auto record = IdempotencyRecord::processing(
        scope(), IdempotencyFingerprint{"fingerprint"}, identifiers(), time_point(9));
    EXPECT_EQ(record.status(), IdempotencyStatus::Processing);
    EXPECT_FALSE(record.result().has_value());
}

TEST(IdempotencyRecordTest, SuccessfulTransitionPreservesImmutableOperationIdentity) {
    auto record = IdempotencyRecord::processing(
        scope(), IdempotencyFingerprint{"fingerprint"}, identifiers(), time_point(9));
    const auto original_scope = record.scope();
    const auto original_fingerprint = record.fingerprint();
    const auto original_identifiers = record.generated_identifiers();
    record.record_succeeded(confirmed_snapshot());
    EXPECT_EQ(record.status(), IdempotencyStatus::Succeeded);
    EXPECT_TRUE(record.result()->is_success());
    EXPECT_EQ(record.scope(), original_scope);
    EXPECT_EQ(record.fingerprint(), original_fingerprint);
    EXPECT_EQ(record.generated_identifiers(), original_identifiers);
}

TEST(IdempotencyRecordTest, PermanentFailureTransitionPreservesImmutableOperationIdentity) {
    auto record = IdempotencyRecord::processing(
        scope(), IdempotencyFingerprint{"fingerprint"}, identifiers(), time_point(9));
    const auto original_scope = record.scope();
    const auto original_fingerprint = record.fingerprint();
    const auto original_identifiers = record.generated_identifiers();
    record.record_failed_permanently(rejection_snapshot());
    EXPECT_EQ(record.status(), IdempotencyStatus::FailedPermanent);
    EXPECT_FALSE(record.result()->is_success());
    EXPECT_EQ(record.scope(), original_scope);
    EXPECT_EQ(record.fingerprint(), original_fingerprint);
    EXPECT_EQ(record.generated_identifiers(), original_identifiers);
}

TEST(IdempotencyRecordTest, TerminalFactoriesRejectMismatchedSnapshots) {
    EXPECT_THROW(
        static_cast<void>(IdempotencyRecord::succeeded(scope(),
                                                       IdempotencyFingerprint{"fingerprint"},
                                                       identifiers(),
                                                       time_point(9),
                                                       rejection_snapshot())),
        std::invalid_argument);
    EXPECT_THROW(static_cast<void>(
                     IdempotencyRecord::failed_permanently(scope(),
                                                           IdempotencyFingerprint{"fingerprint"},
                                                           identifiers(),
                                                           time_point(9),
                                                           confirmed_snapshot())),
                 std::invalid_argument);
}

TEST(IdempotencyRecordTest, CompletedRecordCannotTransitionAgain) {
    auto record = IdempotencyRecord::succeeded(scope(),
                                               IdempotencyFingerprint{"fingerprint"},
                                               identifiers(),
                                               time_point(9),
                                               confirmed_snapshot());
    EXPECT_THROW(record.record_failed_permanently(rejection_snapshot()), std::logic_error);
}

TEST(CreateReservationResultSnapshotTest, RejectsSuccessWithRejectionStatus) {
    EXPECT_THROW(static_cast<void>(CreateReservationResultSnapshot::successful(
                     CreateReservationStatus::RESOURCE_NOT_FOUND,
                     ReservationId{"reservation-1"},
                     ResourceId{"resource-1"},
                     ReservationStatus::Confirmed,
                     ReservationKind::Standard,
                     Version{1},
                     time_point(9))),
                 std::invalid_argument);
}

TEST(CreateReservationResultSnapshotTest, RejectsMismatchedSuccessfulStatuses) {
    EXPECT_THROW(static_cast<void>(CreateReservationResultSnapshot::successful(
                     CreateReservationStatus::CREATED_CONFIRMED,
                     ReservationId{"reservation-1"},
                     ResourceId{"resource-1"},
                     ReservationStatus::PendingApproval,
                     ReservationKind::Standard,
                     Version{1},
                     time_point(9))),
                 std::invalid_argument);
}

TEST(CreateReservationResultSnapshotTest, RejectsZeroInitialVersion) {
    EXPECT_THROW(static_cast<void>(CreateReservationResultSnapshot::successful(
                     CreateReservationStatus::CREATED_CONFIRMED,
                     ReservationId{"reservation-1"},
                     ResourceId{"resource-1"},
                     ReservationStatus::Confirmed,
                     ReservationKind::Standard,
                     Version{0},
                     time_point(9))),
                 std::invalid_argument);
}

TEST(CreateReservationResultSnapshotTest, PermanentFailureRejectsSuccessStatus) {
    EXPECT_THROW(static_cast<void>(CreateReservationResultSnapshot::permanent_rejection(
                     CreateReservationStatus::CREATED_CONFIRMED)),
                 std::invalid_argument);
}

TEST(CreateReservationResultSnapshotTest, ScheduleConflictIsAReplayablePermanentRejection) {
    const auto snapshot = rejection_snapshot();
    EXPECT_FALSE(snapshot.is_success());
    EXPECT_EQ(snapshot.creation_status(), CreateReservationStatus::SCHEDULE_CONFLICT);
    EXPECT_FALSE(snapshot.reservation_id().has_value());
}

TEST(CreateReservationResultSnapshotTest, PendingApprovalPreservesReplayFields) {
    const auto snapshot = CreateReservationResultSnapshot::successful(
        CreateReservationStatus::CREATED_PENDING_APPROVAL,
        ReservationId{"reservation-1"},
        ResourceId{"resource-1"},
        ReservationStatus::PendingApproval,
        ReservationKind::Standard,
        Version{1},
        time_point(9));
    EXPECT_TRUE(snapshot.is_success());
    EXPECT_EQ(snapshot.reservation_status(), ReservationStatus::PendingApproval);
    EXPECT_EQ(snapshot.created_at(), time_point(9));
}

}  // namespace
}  // namespace haven::application::idempotency
