/**
 * @file reservation_document_test.cpp
 * @brief Tests reservation document JSON conversion.
 */

#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] ReservationDocument document(std::string purpose = "  Planning & review  ") {
    return ReservationDocument{
        .schema_version = kReservationDocumentSchemaVersion,
        .reservation_id = "reservation-123",
        .organization_id = "organization-123",
        .resource_id = "resource-123",
        .created_by = "creator-123",
        .start_time = "2026-07-29T04:30:00.123456789Z",
        .end_time = "2026-07-29T05:30:00.987654321Z",
        .purpose = std::move(purpose),
        .status = "CONFIRMED",
        .kind = "MAINTENANCE",
        .approval = ReservationApprovalDocument{.approved_by = "approver-123",
                                                .approved_at = "2026-07-28T03:00:00.000000001Z"},
        .version = 42,
    };
}

TEST(ReservationDocumentTest, SerializesEveryPersistedField) {
    const auto expected = document();
    const auto json = reservation_document_to_json(expected);

    EXPECT_EQ(json.at("documentType").get_string(), kReservationDocumentType);
    EXPECT_EQ(json.at("schemaVersion").get_unsigned(), expected.schema_version);
    EXPECT_EQ(json.at("reservationId").get_string(), expected.reservation_id);
    EXPECT_EQ(json.at("organizationId").get_string(), expected.organization_id);
    EXPECT_EQ(json.at("resourceId").get_string(), expected.resource_id);
    EXPECT_EQ(json.at("createdBy").get_string(), expected.created_by);
    EXPECT_EQ(json.at("startTime").get_string(), expected.start_time);
    EXPECT_EQ(json.at("endTime").get_string(), expected.end_time);
    EXPECT_EQ(json.at("purpose").get_string(), expected.purpose);
    EXPECT_EQ(json.at("status").get_string(), expected.status);
    EXPECT_EQ(json.at("kind").get_string(), expected.kind);
    EXPECT_EQ(json.at("approval").at("approvedBy").get_string(), "approver-123");
    EXPECT_EQ(json.at("approval").at("approvedAt").get_string(), expected.approval->approved_at);
    EXPECT_EQ(json.at("version").get_unsigned(), expected.version);
}

TEST(ReservationDocumentTest, DeserializesAndRoundTripsCompleteDocument) {
    const auto expected = document();
    EXPECT_EQ(reservation_document_from_json(reservation_document_to_json(expected)), expected);
}

TEST(ReservationDocumentTest, SupportsAbsentApproval) {
    auto expected = document();
    expected.approval.reset();
    const auto json = reservation_document_to_json(expected);

    EXPECT_EQ(json.get_object().count("approval"), 0U);
    EXPECT_EQ(reservation_document_from_json(json), expected);
}

TEST(ReservationDocumentTest, RoundTripsRejectionWithReason) {
    auto expected = document();
    expected.approval.reset();
    expected.status = "REJECTED";
    expected.rejection = ReservationRejectionDocument{
        .rejected_by = "approver-123",
        .rejected_at = "2026-07-28T03:00:00.000000001Z",
        .reason = std::string{"Reserved for maintenance."},
    };
    const auto json = reservation_document_to_json(expected);

    EXPECT_EQ(json.at("rejection").at("rejectedBy").get_string(), "approver-123");
    EXPECT_EQ(json.at("rejection").at("reason").get_string(), "Reserved for maintenance.");
    EXPECT_EQ(reservation_document_from_json(json), expected);
}

TEST(ReservationDocumentTest, RoundTripsRejectionWithoutReason) {
    auto expected = document();
    expected.approval.reset();
    expected.status = "REJECTED";
    expected.rejection = ReservationRejectionDocument{
        .rejected_by = "approver-123",
        .rejected_at = "2026-07-28T03:00:00.000000001Z",
        .reason = std::nullopt,
    };
    const auto json = reservation_document_to_json(expected);

    EXPECT_EQ(json.at("rejection").get_object().count("reason"), 0U);
    EXPECT_EQ(reservation_document_from_json(json), expected);
}

TEST(ReservationDocumentTest, RoundTripsCancellationWithReason) {
    auto expected = document();
    expected.approval.reset();
    expected.status = "CANCELLED";
    expected.cancellation = ReservationCancellationDocument{
        .cancelled_by = "admin-123",
        .cancelled_at = "2026-07-28T03:00:00.000000001Z",
        .reason = std::string{"Double-booked by mistake."},
    };
    const auto json = reservation_document_to_json(expected);

    EXPECT_EQ(json.at("cancellation").at("cancelledBy").get_string(), "admin-123");
    EXPECT_EQ(json.at("cancellation").at("reason").get_string(), "Double-booked by mistake.");
    EXPECT_EQ(reservation_document_from_json(json), expected);
}

TEST(ReservationDocumentTest, RoundTripsCancellationWithoutReason) {
    auto expected = document();
    expected.approval.reset();
    expected.status = "CANCELLED";
    expected.cancellation = ReservationCancellationDocument{
        .cancelled_by = "admin-123",
        .cancelled_at = "2026-07-28T03:00:00.000000001Z",
        .reason = std::nullopt,
    };
    const auto json = reservation_document_to_json(expected);

    EXPECT_EQ(json.at("cancellation").get_object().count("reason"), 0U);
    EXPECT_EQ(reservation_document_from_json(json), expected);
}

TEST(ReservationDocumentTest, RejectsIncorrectAndMissingDocumentType) {
    auto incorrect = reservation_document_to_json(document());
    incorrect["documentType"] = "resource";
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(incorrect)),
                 std::invalid_argument);

    auto missing = reservation_document_to_json(document());
    missing.erase("documentType");
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(missing)), std::exception);
}

TEST(ReservationDocumentTest, RejectsMissingAndUnsupportedSchemaVersion) {
    auto missing = reservation_document_to_json(document());
    missing.erase("schemaVersion");
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(missing)), std::exception);

    auto unsupported = reservation_document_to_json(document());
    unsupported["schemaVersion"] = kReservationDocumentSchemaVersion + 1;
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(unsupported)),
                 std::invalid_argument);
}

TEST(ReservationDocumentTest, RejectsMissingRequiredFieldAndIncorrectType) {
    auto missing = reservation_document_to_json(document());
    missing.erase("reservationId");
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(missing)), std::exception);

    auto incorrect = reservation_document_to_json(document());
    incorrect["startTime"] = 123;
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(incorrect)), std::exception);
}

TEST(ReservationDocumentTest, RejectsMalformedTimestampAndIncompleteApproval) {
    auto malformed = reservation_document_to_json(document());
    malformed["endTime"] = "not-a-time";
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(malformed)),
                 std::invalid_argument);

    auto incomplete = reservation_document_to_json(document());
    incomplete.at("approval").erase("approvedAt");
    EXPECT_THROW(static_cast<void>(reservation_document_from_json(incomplete)), std::exception);
}

TEST(ReservationDocumentTest, IgnoresUnknownAdditiveFields) {
    auto json = reservation_document_to_json(document());
    json["futureField"] = "future-value";
    EXPECT_EQ(reservation_document_from_json(json), document());
}

TEST(ReservationDocumentTest, PreservesPermissivePurposeTextExactly) {
    for (const std::string purpose : {"", "   ", "  exact\ntext  "}) {
        const auto expected = document(purpose);
        EXPECT_EQ(reservation_document_from_json(reservation_document_to_json(expected)).purpose,
                  purpose);
    }
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
