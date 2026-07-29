/**
 * @file reservation_document_validator_test.cpp
 * @brief Tests structural reservation document validation.
 */

#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] ReservationDocument valid_document() {
    return ReservationDocument{
        .schema_version = kReservationDocumentSchemaVersion,
        .reservation_id = "reservation-123",
        .organization_id = "organization-123",
        .resource_id = "resource-123",
        .created_by = "creator-123",
        .start_time = "2026-07-29T04:30:00.123456789Z",
        .end_time = "2026-07-29T05:30:00.123456789Z",
        .purpose = "",
        .status = "CONFIRMED",
        .kind = "STANDARD",
        .approval = std::nullopt,
        .version = 1,
    };
}

TEST(ReservationDocumentValidatorTest, AcceptsValidDocumentAndPermissivePurposes) {
    EXPECT_NO_THROW(validate_reservation_document(valid_document()));
    auto whitespace = valid_document();
    whitespace.purpose = "   ";
    EXPECT_NO_THROW(validate_reservation_document(whitespace));
}

TEST(ReservationDocumentValidatorTest, RejectsUnsupportedSchemaAndZeroVersion) {
    auto schema = valid_document();
    schema.schema_version = 2;
    EXPECT_THROW(validate_reservation_document(schema), std::invalid_argument);
    auto version = valid_document();
    version.version = 0;
    EXPECT_THROW(validate_reservation_document(version), std::invalid_argument);
}

TEST(ReservationDocumentValidatorTest, RejectsEmptyIdentifiersStatusAndKind) {
    auto value = valid_document();
    value.reservation_id.clear();
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
    value = valid_document();
    value.organization_id.clear();
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
    value = valid_document();
    value.resource_id.clear();
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
    value = valid_document();
    value.created_by.clear();
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
    value = valid_document();
    value.status.clear();
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
    value = valid_document();
    value.kind.clear();
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
}

TEST(ReservationDocumentValidatorTest, RejectsMalformedStartAndEnd) {
    auto start = valid_document();
    start.start_time = "2026-02-30T04:30:00.000000000Z";
    EXPECT_THROW(validate_reservation_document(start), std::invalid_argument);
    auto end = valid_document();
    end.end_time = "2026-07-29 05:30:00Z";
    EXPECT_THROW(validate_reservation_document(end), std::invalid_argument);
}

TEST(ReservationDocumentValidatorTest, RejectsEqualOrReversedInterval) {
    auto equal = valid_document();
    equal.end_time = equal.start_time;
    EXPECT_THROW(validate_reservation_document(equal), std::invalid_argument);
    auto reversed = valid_document();
    reversed.end_time = "2026-07-29T03:30:00.123456789Z";
    EXPECT_THROW(validate_reservation_document(reversed), std::invalid_argument);
}

TEST(ReservationDocumentValidatorTest, RejectsIncompleteApproval) {
    auto value = valid_document();
    value.approval = ReservationApprovalDocument{"", "2026-07-29T03:30:00.000000000Z"};
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
    value.approval = ReservationApprovalDocument{"approver-123", "invalid"};
    EXPECT_THROW(validate_reservation_document(value), std::invalid_argument);
}

TEST(ReservationDocumentValidatorTest, TimestampConversionRoundTripsExactInstant) {
    using namespace std::chrono;
    const auto expected = system_clock::time_point{} + hours{123456} + microseconds{123456};
    EXPECT_EQ(reservation_timestamp_from_string(reservation_timestamp_to_string(expected)),
              expected);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
