/**
 * @file reservation_document_mapper_test.cpp
 * @brief Tests reservation aggregate persistence mapping.
 */

#include "haven/infrastructure/persistence/couchbase/reservation_document_mapper.hpp"

#include "haven/domain/value_objects/rejection_info.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using namespace std::chrono_literals;

[[nodiscard]] domain::Reservation reservation(
    const domain::ReservationStatus status,
    const domain::ReservationKind kind = domain::ReservationKind::Maintenance,
    std::optional<domain::ApprovalInfo> approval = std::nullopt,
    std::string purpose = "  exact purpose  ",
    std::optional<domain::RejectionInfo> rejection = std::nullopt) {
    const auto start = domain::TimeInterval::TimePoint{} + 100h + 123456us;
    return domain::Reservation::rehydrate(domain::OrganizationId{"organization-123"},
                                          domain::ReservationId{"reservation-123"},
                                          domain::ResourceId{"resource-123"},
                                          domain::UserId{"creator-123"},
                                          domain::TimeInterval{start, start + 90min},
                                          domain::Purpose{std::move(purpose)},
                                          kind,
                                          status,
                                          std::move(approval),
                                          std::move(rejection),
                                          domain::Version{42});
}

TEST(ReservationDocumentMapperTest, MapsEveryAggregateField) {
    const auto approved_at = domain::ApprovalInfo::TimePoint{} + 90h + 1us;
    const auto mapped = to_reservation_document(
        reservation(domain::ReservationStatus::Confirmed,
                    domain::ReservationKind::Maintenance,
                    domain::ApprovalInfo{domain::UserId{"approver-123"}, approved_at}));

    EXPECT_EQ(mapped.schema_version, kReservationDocumentSchemaVersion);
    EXPECT_EQ(mapped.reservation_id, "reservation-123");
    EXPECT_EQ(mapped.organization_id, "organization-123");
    EXPECT_EQ(mapped.resource_id, "resource-123");
    EXPECT_EQ(mapped.created_by, "creator-123");
    EXPECT_EQ(mapped.purpose, "  exact purpose  ");
    EXPECT_EQ(mapped.status, "CONFIRMED");
    EXPECT_EQ(mapped.kind, "MAINTENANCE");
    ASSERT_TRUE(mapped.approval.has_value());
    EXPECT_EQ(mapped.approval->approved_by, "approver-123");
    EXPECT_EQ(reservation_timestamp_from_string(mapped.approval->approved_at), approved_at);
    EXPECT_EQ(mapped.version, 42U);
}

TEST(ReservationDocumentMapperTest, PreservesEveryLifecycleStateWithoutEvents) {
    constexpr std::array statuses{domain::ReservationStatus::PendingApproval,
                                  domain::ReservationStatus::Confirmed,
                                  domain::ReservationStatus::Cancelled,
                                  domain::ReservationStatus::Rejected,
                                  domain::ReservationStatus::Expired,
                                  domain::ReservationStatus::Completed};

    for (const auto status : statuses) {
        auto restored = to_domain_reservation(to_reservation_document(reservation(status)));
        EXPECT_EQ(restored.status(), status);
        EXPECT_TRUE(restored.release_domain_events().empty());
    }
}

TEST(ReservationDocumentMapperTest, RoundTripPreservesAllPersistedState) {
    const auto approved_at = domain::ApprovalInfo::TimePoint{} + 90h + 7us;
    const auto original =
        reservation(domain::ReservationStatus::Cancelled,
                    domain::ReservationKind::Standard,
                    domain::ApprovalInfo{domain::UserId{"approver-123"}, approved_at},
                    " \n ");
    const auto restored = to_domain_reservation(to_reservation_document(original));

    EXPECT_EQ(restored.organization_id(), original.organization_id());
    EXPECT_EQ(restored.reservation_id(), original.reservation_id());
    EXPECT_EQ(restored.resource_id(), original.resource_id());
    EXPECT_EQ(restored.created_by(), original.created_by());
    EXPECT_EQ(restored.interval(), original.interval());
    EXPECT_EQ(restored.purpose(), original.purpose());
    EXPECT_EQ(restored.kind(), original.kind());
    EXPECT_EQ(restored.status(), original.status());
    EXPECT_EQ(restored.approval_info(), original.approval_info());
    EXPECT_EQ(restored.version(), original.version());
}

TEST(ReservationDocumentMapperTest, MapsRejectionReasonWhenPresent) {
    const auto rejected_at = domain::RejectionInfo::TimePoint{} + 90h + 3us;
    const auto mapped = to_reservation_document(
        reservation(domain::ReservationStatus::Rejected,
                    domain::ReservationKind::Maintenance,
                    std::nullopt,
                    "  exact purpose  ",
                    domain::RejectionInfo{domain::UserId{"approver-123"},
                                         rejected_at,
                                         std::string{"Reserved for maintenance."}}));

    ASSERT_TRUE(mapped.rejection.has_value());
    EXPECT_EQ(mapped.rejection->rejected_by, "approver-123");
    EXPECT_EQ(reservation_timestamp_from_string(mapped.rejection->rejected_at), rejected_at);
    ASSERT_TRUE(mapped.rejection->reason.has_value());
    EXPECT_EQ(*mapped.rejection->reason, "Reserved for maintenance.");

    const auto restored = to_domain_reservation(mapped);
    ASSERT_TRUE(restored.rejection_info().has_value());
    EXPECT_EQ(*restored.rejection_info()->reason(), "Reserved for maintenance.");
}

TEST(ReservationDocumentMapperTest, RejectsUnknownStatusAndKind) {
    auto status = to_reservation_document(reservation(domain::ReservationStatus::Confirmed));
    status.status = "UNKNOWN";
    EXPECT_THROW(static_cast<void>(to_domain_reservation(status)), std::invalid_argument);
    auto kind = to_reservation_document(reservation(domain::ReservationStatus::Confirmed));
    kind.kind = "UNKNOWN";
    EXPECT_THROW(static_cast<void>(to_domain_reservation(kind)), std::invalid_argument);
}

TEST(ReservationDocumentMapperTest, RejectsMalformedTimestampAndUnsupportedSchema) {
    auto timestamp = to_reservation_document(reservation(domain::ReservationStatus::Confirmed));
    timestamp.start_time = "invalid";
    EXPECT_THROW(static_cast<void>(to_domain_reservation(timestamp)), std::invalid_argument);
    auto schema = to_reservation_document(reservation(domain::ReservationStatus::Confirmed));
    schema.schema_version = 2;
    EXPECT_THROW(static_cast<void>(to_domain_reservation(schema)), std::invalid_argument);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
