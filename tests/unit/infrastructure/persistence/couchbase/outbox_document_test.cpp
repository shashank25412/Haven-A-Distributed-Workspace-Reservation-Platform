/**
 * @file outbox_document_test.cpp
 * @brief Tests Reservation Outbox mapping, JSON conversion, and validation.
 */

#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"

#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_validator.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using namespace std::chrono_literals;
using haven::domain::EventId;
using haven::domain::OrganizationId;
using haven::domain::ReservationDomainEvent;
using haven::domain::ReservationId;
using haven::domain::ReservationStatus;
using haven::domain::ResourceId;
using haven::domain::TimeInterval;
using haven::domain::UserId;

constexpr auto kOccurredAt = std::chrono::system_clock::time_point{} + 30min;
const auto kOrganizationId = OrganizationId{"organization-1"};
const auto kReservationId = ReservationId{"reservation-1"};
const auto kResourceId = ResourceId{"resource-1"};
const auto kInterval = TimeInterval{kOccurredAt + 1h, kOccurredAt + 2h};

[[nodiscard]] ReservationDomainEvent created_event() {
    return haven::domain::ReservationCreatedEvent{EventId{"created-1"},
                                                  kOccurredAt,
                                                  kOrganizationId,
                                                  kReservationId,
                                                  kResourceId,
                                                  UserId{"creator-1"},
                                                  kInterval,
                                                  haven::domain::ReservationKind::Standard,
                                                  ReservationStatus::Confirmed};
}

[[nodiscard]] OutboxDocument created_document() {
    return to_outbox_document(kOrganizationId, kReservationId, created_event());
}

TEST(OutboxStatusTest, SerializesAndParsesEveryStatus) {
    EXPECT_EQ(to_string(OutboxStatus::Pending), "PENDING");
    EXPECT_EQ(to_string(OutboxStatus::Publishing), "PUBLISHING");
    EXPECT_EQ(to_string(OutboxStatus::Published), "PUBLISHED");
    EXPECT_EQ(outbox_status_from_string("PENDING"), OutboxStatus::Pending);
    EXPECT_EQ(outbox_status_from_string("PUBLISHING"), OutboxStatus::Publishing);
    EXPECT_EQ(outbox_status_from_string("PUBLISHED"), OutboxStatus::Published);
    EXPECT_THROW(static_cast<void>(outbox_status_from_string("UNKNOWN")), std::invalid_argument);
}

TEST(OutboxDocumentMapperTest, MapsCreatedEventEnvelopeAndCompletePayload) {
    const auto document = created_document();

    EXPECT_EQ(document.schema_version, kOutboxDocumentSchemaVersion);
    EXPECT_EQ(document.event_id, EventId{"created-1"});
    EXPECT_EQ(document.organization_id, kOrganizationId);
    EXPECT_EQ(document.aggregate_id, kReservationId);
    EXPECT_EQ(document.aggregate_type, kReservationAggregateType);
    EXPECT_EQ(document.event_type, kReservationCreatedEventType);
    EXPECT_EQ(document.occurred_at, kOccurredAt);
    EXPECT_EQ(document.status, OutboxStatus::Pending);
    EXPECT_EQ(document.attempt_count, 0U);
    EXPECT_FALSE(document.published_at.has_value());
    EXPECT_EQ(document.payload.at("resourceId").get_string(), kResourceId.value());
    EXPECT_EQ(document.payload.at("createdBy").get_string(), "creator-1");
    EXPECT_EQ(document.payload.at("startTime").get_string(),
              reservation_timestamp_to_string(kInterval.start()));
    EXPECT_EQ(document.payload.at("endTime").get_string(),
              reservation_timestamp_to_string(kInterval.end()));
    EXPECT_EQ(document.payload.at("kind").get_string(), "STANDARD");
    EXPECT_EQ(document.payload.at("initialStatus").get_string(), "CONFIRMED");
}

TEST(OutboxDocumentMapperTest, MapsAllReservationEventVariants) {
    const auto previous = TimeInterval{kOccurredAt + 1h, kOccurredAt + 2h};
    const auto extended = TimeInterval{kOccurredAt + 1h, kOccurredAt + 3h};
    const auto cases = std::vector<std::pair<ReservationDomainEvent, std::string>>{
        {haven::domain::ReservationConfirmedEvent{EventId{"confirmed"},
                                                  kOccurredAt,
                                                  kOrganizationId,
                                                  kReservationId,
                                                  kResourceId,
                                                  kInterval,
                                                  UserId{"approver"}},
         kReservationConfirmedEventType},
        {haven::domain::ReservationApprovalRequestedEvent{
             EventId{"approval-requested"},
             kOccurredAt,
             kOrganizationId,
             kReservationId,
             kResourceId,
             UserId{"requester"},
             kInterval,
             haven::domain::ReservationKind::Maintenance},
         kReservationApprovalRequestedEventType},
        {haven::domain::ReservationRejectedEvent{EventId{"rejected"},
                                                 kOccurredAt,
                                                 kOrganizationId,
                                                 kReservationId,
                                                 kResourceId,
                                                 UserId{"rejector"}},
         kReservationRejectedEventType},
        {haven::domain::ReservationCancelledEvent{EventId{"cancelled"},
                                                  kOccurredAt,
                                                  kOrganizationId,
                                                  kReservationId,
                                                  kResourceId,
                                                  UserId{"canceller"},
                                                  ReservationStatus::Confirmed},
         kReservationCancelledEventType},
        {haven::domain::ReservationExtendedEvent{EventId{"extended"},
                                                 kOccurredAt,
                                                 kOrganizationId,
                                                 kReservationId,
                                                 kResourceId,
                                                 UserId{"extender"},
                                                 previous,
                                                 extended},
         kReservationExtendedEventType},
        {haven::domain::ReservationExpiredEvent{EventId{"expired"},
                                                kOccurredAt,
                                                kOrganizationId,
                                                kReservationId,
                                                kResourceId,
                                                ReservationStatus::PendingApproval},
         kReservationExpiredEventType},
        {haven::domain::ReservationCompletedEvent{EventId{"completed"},
                                                  kOccurredAt,
                                                  kOrganizationId,
                                                  kReservationId,
                                                  kResourceId,
                                                  kInterval},
         kReservationCompletedEventType}};

    for (const auto& [event, expected_type] : cases) {
        const auto document = to_outbox_document(kOrganizationId, kReservationId, event);
        EXPECT_EQ(document.event_type, expected_type);
        EXPECT_TRUE(document.payload.get_object().contains("resourceId"));
    }

    const auto confirmed = to_outbox_document(kOrganizationId, kReservationId, cases[0].first);
    EXPECT_EQ(confirmed.payload.at("confirmedBy").get_string(), "approver");
    const auto requested = to_outbox_document(kOrganizationId, kReservationId, cases[1].first);
    EXPECT_EQ(requested.payload.at("requestedBy").get_string(), "requester");
    EXPECT_EQ(requested.payload.at("kind").get_string(), "MAINTENANCE");
    const auto rejected = to_outbox_document(kOrganizationId, kReservationId, cases[2].first);
    EXPECT_EQ(rejected.payload.at("rejectedBy").get_string(), "rejector");
    const auto cancelled = to_outbox_document(kOrganizationId, kReservationId, cases[3].first);
    EXPECT_EQ(cancelled.payload.at("cancelledBy").get_string(), "canceller");
    EXPECT_EQ(cancelled.payload.at("previousStatus").get_string(), "CONFIRMED");
    const auto extended_document =
        to_outbox_document(kOrganizationId, kReservationId, cases[4].first);
    EXPECT_EQ(extended_document.payload.at("extendedBy").get_string(), "extender");
    EXPECT_EQ(extended_document.payload.at("extendedEndTime").get_string(),
              reservation_timestamp_to_string(extended.end()));
    const auto expired = to_outbox_document(kOrganizationId, kReservationId, cases[5].first);
    EXPECT_EQ(expired.payload.at("previousStatus").get_string(), "PENDING_APPROVAL");
    const auto completed = to_outbox_document(kOrganizationId, kReservationId, cases[6].first);
    EXPECT_EQ(completed.payload.at("endTime").get_string(),
              reservation_timestamp_to_string(kInterval.end()));
}

TEST(OutboxDocumentMapperTest, RejectsEventIdentityDifferentFromSuppliedAggregate) {
    EXPECT_THROW(static_cast<void>(
                     to_outbox_document(OrganizationId{"other"}, kReservationId, created_event())),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(
                     to_outbox_document(kOrganizationId, ReservationId{"other"}, created_event())),
                 std::invalid_argument);
}

TEST(OutboxDocumentJsonTest, RoundTripsCreatedAndOptionalConfirmedPayloads) {
    const auto created = created_document();
    EXPECT_EQ(outbox_document_from_json(outbox_document_to_json(created)), created);

    const auto automatic_confirmation =
        ReservationDomainEvent{haven::domain::ReservationConfirmedEvent{EventId{"confirmed"},
                                                                        kOccurredAt,
                                                                        kOrganizationId,
                                                                        kReservationId,
                                                                        kResourceId,
                                                                        kInterval,
                                                                        std::nullopt}};
    const auto confirmed =
        to_outbox_document(kOrganizationId, kReservationId, automatic_confirmation);
    EXPECT_FALSE(confirmed.payload.get_object().contains("confirmedBy"));
    EXPECT_EQ(outbox_document_from_json(outbox_document_to_json(confirmed)), confirmed);
}

TEST(OutboxDocumentJsonTest, RoundTripsPublishedDocument) {
    auto document = created_document();
    document.status = OutboxStatus::Published;
    document.attempt_count = 3;
    document.published_at = kOccurredAt + 5min;

    const auto json = outbox_document_to_json(document);

    EXPECT_EQ(json.at("status").get_string(), "PUBLISHED");
    EXPECT_EQ(json.at("attemptCount").get_unsigned(), 3U);
    EXPECT_EQ(outbox_document_from_json(json), document);
}

TEST(OutboxDocumentValidationTest, RejectsInvalidIdentitySchemaTypeStatusAndPayload) {
    auto json = outbox_document_to_json(created_document());

    auto invalid = json;
    invalid["documentType"] = "reservation";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["schemaVersion"] = 2;
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["eventId"] = "";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["organizationId"] = "";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["aggregateId"] = "";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["aggregateType"] = "Resource";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["eventType"] = "Unknown";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["status"] = "UNKNOWN";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid.get_object().erase("payload");
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::exception);
    invalid = json;
    invalid["payload"] = tao::json::empty_array;
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::exception);
}

TEST(OutboxDocumentValidationTest, RejectsInvalidPublicationStateAttemptAndTimestamps) {
    auto json = outbox_document_to_json(created_document());

    auto invalid = json;
    invalid["status"] = "PUBLISHED";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["publishedAt"] = reservation_timestamp_to_string(kOccurredAt);
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["status"] = "PUBLISHING";
    invalid["publishedAt"] = reservation_timestamp_to_string(kOccurredAt);
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["attemptCount"] = -1;
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::exception);
    invalid = json;
    invalid["attemptCount"] =
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1U;
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["occurredAt"] = "not-a-timestamp";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
    invalid = json;
    invalid["payload"]["startTime"] = "not-a-timestamp";
    EXPECT_THROW(static_cast<void>(outbox_document_from_json(invalid)), std::invalid_argument);
}

TEST(OutboxDocumentKeyTest, IsDeterministicAndSeparatesOrganizationAndEventIdentity) {
    const auto first = outbox_document_key(kOrganizationId, EventId{"event-1"});

    EXPECT_EQ(first, "outbox::organization-1::event-1");
    EXPECT_EQ(outbox_document_key(kOrganizationId, EventId{"event-1"}), first);
    EXPECT_NE(outbox_document_key(OrganizationId{"organization-2"}, EventId{"event-1"}), first);
    EXPECT_NE(outbox_document_key(kOrganizationId, EventId{"event-2"}), first);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase
