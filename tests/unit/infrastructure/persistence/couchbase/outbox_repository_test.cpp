/** @file outbox_repository_test.cpp @brief Tests transport-neutral Outbox mapping. */
#include "haven/domain/events/reservation_created_event.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_message_mapper.hpp"

#include <gtest/gtest.h>

#include <tao/json.hpp>

namespace haven::infrastructure::persistence::couchbase {
TEST(OutboxMessageMapperTest, ProducesCanonicalEnvelopeIndependentOfLifecycleState) {
    const auto now = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}};
    const auto organization = haven::domain::OrganizationId{"org"};
    const auto reservation = haven::domain::ReservationId{"reservation"};
    const auto event = haven::domain::ReservationCreatedEvent{
        haven::domain::EventId{"event"},
        now,
        organization,
        reservation,
        haven::domain::ResourceId{"resource"},
        haven::domain::UserId{"user"},
        haven::domain::TimeInterval{now, now + std::chrono::hours{1}},
        haven::domain::ReservationKind::Standard,
        haven::domain::ReservationStatus::Confirmed};
    auto document = to_outbox_document(organization, reservation, event);
    const auto pending = to_outbox_message(document);
    document.status = OutboxStatus::Publishing;
    document.attempt_count = 1;
    const auto publishing = to_outbox_message(document);
    EXPECT_EQ(pending.serialized_envelope, publishing.serialized_envelope);
    EXPECT_EQ(publishing.attempt_count, 1U);
    const auto envelope = tao::json::from_string(pending.serialized_envelope);
    EXPECT_EQ(envelope.at("eventId").get_string(), "event");
    EXPECT_TRUE(envelope.at("payload").is_object());
    EXPECT_FALSE(envelope.get_object().contains("status"));
    EXPECT_FALSE(envelope.get_object().contains("attemptCount"));
}
}  // namespace haven::infrastructure::persistence::couchbase
