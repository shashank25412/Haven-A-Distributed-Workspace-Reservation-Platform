/**
 * @file idempotency_persistence_test.cpp
 * @brief Tests Couchbase idempotency document mapping, validation, and keys.
 */

#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_validator.hpp"

#include <gtest/gtest.h>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using namespace haven::application::idempotency;
using namespace haven::application::reservations;

IdempotencyScope scope(IdempotencyOperation operation = IdempotencyOperation::CreateReservation,
                       std::string key = "key") {
    return {haven::domain::OrganizationId{"org"},
            haven::domain::UserId{"user"},
            operation,
            haven::domain::IdempotencyKey{std::move(key)}};
}

IdempotencyRecord processing() {
    return IdempotencyRecord::processing(
        scope(),
        IdempotencyFingerprint{std::string(64, 'a')},
        {haven::domain::ReservationId{"reservation"},
         haven::domain::EventId{"created"},
         haven::domain::EventId{"confirmed"},
         haven::domain::EventId{"approval"}},
        IdempotencyRecord::TimePoint{std::chrono::seconds{1'800'000'000}});
}

CreateReservationResultSnapshot success() {
    return CreateReservationResultSnapshot::successful(
        CreateReservationStatus::CREATED_CONFIRMED,
        haven::domain::OrganizationId{"org"},
        haven::domain::ReservationId{"reservation"},
        haven::domain::ResourceId{"resource"},
        haven::domain::UserId{"user"},
        haven::domain::TimeInterval{
            CreateReservationResultSnapshot::TimePoint{std::chrono::seconds{1'800'000'000}},
            CreateReservationResultSnapshot::TimePoint{std::chrono::seconds{1'800'000'100}}},
        haven::domain::Purpose{"purpose"},
        haven::domain::ReservationStatus::Confirmed,
        haven::domain::ReservationKind::Standard,
        haven::domain::Version{1},
        CreateReservationResultSnapshot::TimePoint{std::chrono::seconds{1'800'000'000}});
}

TEST(IdempotencyDocumentKeyTest, UsesStableGoldenHash) {
    EXPECT_EQ(idempotency_document_key(scope()),
              "idem::34073f702547bfd3f99c2487f5fc70f4819afa504f06c875fed2caedfa871b84");
}

TEST(IdempotencyDocumentKeyTest, SeparatesEveryScopeField) {
    const auto baseline = idempotency_document_key(scope());
    EXPECT_NE(baseline,
              idempotency_document_key({haven::domain::OrganizationId{"other"},
                                        haven::domain::UserId{"user"},
                                        IdempotencyOperation::CreateReservation,
                                        haven::domain::IdempotencyKey{"key"}}));
    EXPECT_NE(baseline,
              idempotency_document_key({haven::domain::OrganizationId{"org"},
                                        haven::domain::UserId{"other"},
                                        IdempotencyOperation::CreateReservation,
                                        haven::domain::IdempotencyKey{"key"}}));
    EXPECT_NE(baseline, idempotency_document_key(scope(IdempotencyOperation::ExtendReservation)));
    EXPECT_NE(baseline,
              idempotency_document_key(scope(IdempotencyOperation::CreateReservation, "other")));
}

TEST(IdempotencyDocumentTest, SerializesCanonicalOperationsAndStatuses) {
    EXPECT_EQ(idempotency_operation_to_string(IdempotencyOperation::CreateReservation),
              "CREATE_RESERVATION");
    EXPECT_EQ(idempotency_operation_from_string("EXTEND_RESERVATION"),
              IdempotencyOperation::ExtendReservation);
    EXPECT_EQ(idempotency_status_to_string(IdempotencyStatus::FailedPermanent), "FAILED_PERMANENT");
    EXPECT_EQ(idempotency_status_from_string("SUCCEEDED"), IdempotencyStatus::Succeeded);
    EXPECT_THROW(idempotency_operation_from_string("UNKNOWN"), std::invalid_argument);
    EXPECT_THROW(idempotency_status_from_string("UNKNOWN"), std::invalid_argument);
}

TEST(IdempotencyDocumentTest, RoundTripsProcessingAndTerminalRecords) {
    const auto initial = processing();
    const auto processing_document = to_idempotency_document(initial);
    EXPECT_EQ(idempotency_document_from_json(idempotency_document_to_json(processing_document)),
              processing_document);
    const auto restored = to_idempotency_record(processing_document);
    EXPECT_EQ(restored.scope(), initial.scope());
    EXPECT_EQ(restored.generated_identifiers(), initial.generated_identifiers());
    EXPECT_EQ(restored.created_at(), initial.created_at());
    auto terminal = initial;
    terminal.record_succeeded(success());
    const auto terminal_restored = to_idempotency_record(to_idempotency_document(terminal));
    EXPECT_EQ(terminal_restored.status(), IdempotencyStatus::Succeeded);
    ASSERT_TRUE(terminal_restored.result());
    EXPECT_EQ(*terminal_restored.result(), success());
}

TEST(IdempotencyDocumentTest, RoundTripsPermanentRejection) {
    auto record = processing();
    const auto rejection = CreateReservationResultSnapshot::permanent_rejection(
        CreateReservationStatus::POLICY_REJECTED);
    record.record_failed_permanently(rejection);
    const auto restored = to_idempotency_record(to_idempotency_document(record));
    EXPECT_EQ(restored.status(), IdempotencyStatus::FailedPermanent);
    EXPECT_EQ(*restored.result(), rejection);
}

TEST(IdempotencyDocumentValidatorTest, RejectsMalformedDocuments) {
    auto document = to_idempotency_document(processing());
    document.schema_version = 2;
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
    document = to_idempotency_document(processing());
    document.organization_id.clear();
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
    document = to_idempotency_document(processing());
    document.fingerprint = std::string(64, 'A');
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
    document = to_idempotency_document(processing());
    document.operation = "UNKNOWN";
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
    document = to_idempotency_document(processing());
    document.status = "UNKNOWN";
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
    document = to_idempotency_document(processing());
    document.result = IdempotencyResultDocument{.creation_status = "POLICY_REJECTED"};
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
}

TEST(IdempotencyDocumentValidatorTest, RejectsTerminalSnapshotMismatch) {
    auto document = to_idempotency_document(processing());
    document.status = "SUCCEEDED";
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
    document.status = "FAILED_PERMANENT";
    document.result = IdempotencyResultDocument{.creation_status = "CREATED_CONFIRMED"};
    EXPECT_THROW(validate_idempotency_document(document), std::invalid_argument);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
