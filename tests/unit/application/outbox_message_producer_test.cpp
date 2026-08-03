/** @file outbox_message_producer_test.cpp @brief Tests the recording producer test double. */
#include "application/util/test_outbox_message_producer.hpp"

#include <gtest/gtest.h>

namespace haven::application::outbox::test {
namespace {
OutboxMessage message(std::string event) {
    return {.event_id = haven::domain::EventId{std::move(event)},
            .organization_id = haven::domain::OrganizationId{"org"},
            .aggregate_id = haven::domain::ReservationId{"reservation"},
            .aggregate_type = "Reservation",
            .event_type = "ReservationCreated",
            .occurred_at = {},
            .schema_version = 1,
            .serialized_envelope = "{}",
            .attempt_count = 0};
}
}  // namespace

TEST(TestOutboxMessageProducerTest, RecordsMessagesInPublicationOrder) {
    auto producer = TestOutboxMessageProducer{};
    producer.publish(message("one"));
    producer.publish(message("two"));
    ASSERT_EQ(producer.published_messages.size(), 2U);
    EXPECT_EQ(producer.published_messages[0].event_id, haven::domain::EventId{"one"});
    EXPECT_EQ(producer.published_messages[1].event_id, haven::domain::EventId{"two"});
}

TEST(TestOutboxMessageProducerTest, RecordsAttemptAndThrowsConfiguredFailure) {
    auto producer = TestOutboxMessageProducer{};
    producer.failure.emplace(MessagePublishErrorCode::Unavailable, "unavailable");
    EXPECT_THROW(producer.publish(message("one")), MessagePublishError);
    EXPECT_EQ(producer.published_messages.size(), 1U);
}

}  // namespace haven::application::outbox::test
