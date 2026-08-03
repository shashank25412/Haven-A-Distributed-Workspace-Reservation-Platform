/** @file kafka_outbox_record_test.cpp @brief Tests pure Kafka record mapping. */
#include "haven/infrastructure/messaging/kafka/kafka_outbox_record.hpp"

#include "haven/application/outbox/message_publish_error.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>

namespace haven::infrastructure::messaging::kafka {
namespace {
KafkaProducerConfiguration configuration() {
    return {.enabled = true,
            .brokers = "broker:9092",
            .reservation_events_topic = "reservation-events",
            .client_id = "haven-test",
            .acknowledgement_timeout = std::chrono::seconds{1},
            .delivery_timeout = std::chrono::seconds{2}};
}
haven::application::outbox::OutboxMessage message(std::string organization = "org") {
    return {
        .event_id = haven::domain::EventId{"event"},
        .organization_id = haven::domain::OrganizationId{std::move(organization)},
        .aggregate_id = haven::domain::ReservationId{"reservation"},
        .aggregate_type = "Reservation",
        .event_type = "ReservationCreated",
        .occurred_at = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}},
        .schema_version = 1,
        .serialized_envelope = R"({"eventId":"event"})",
        .attempt_count = 7};
}
}  // namespace

TEST(KafkaOutboxRecordTest, MapsStableKeyCanonicalPayloadAndExactHeaders) {
    const auto source = message();
    const auto record = to_kafka_outbox_record(source, configuration());
    EXPECT_EQ(record.topic, "reservation-events");
    EXPECT_EQ(record.key, "org::reservation");
    EXPECT_EQ(record.payload, source.serialized_envelope);
    EXPECT_EQ(record.headers.size(), 7U);
    EXPECT_EQ(record.headers[0], std::make_pair(std::string{"event-id"}, std::string{"event"}));
    EXPECT_EQ(record.headers[1],
              std::make_pair(std::string{"organization-id"}, std::string{"org"}));
    EXPECT_EQ(record.headers[6].first, "occurred-at");
    EXPECT_TRUE(std::none_of(record.headers.begin(), record.headers.end(), [](const auto& header) {
        return header.first == "status" || header.first == "attempt-count";
    }));
}

TEST(KafkaOutboxRecordTest, TenantScopeChangesAggregateKey) {
    EXPECT_NE(to_kafka_outbox_record(message("one"), configuration()).key,
              to_kafka_outbox_record(message("two"), configuration()).key);
}

TEST(KafkaOutboxRecordTest, RejectsEmptyCanonicalEnvelope) {
    auto source = message();
    source.serialized_envelope.clear();
    try {
        static_cast<void>(to_kafka_outbox_record(source, configuration()));
        FAIL() << "Expected invalid-message failure";
    } catch (const haven::application::outbox::MessagePublishError& error) {
        EXPECT_EQ(error.code(),
                  haven::application::outbox::MessagePublishErrorCode::InvalidMessage);
    }
}

}  // namespace haven::infrastructure::messaging::kafka
