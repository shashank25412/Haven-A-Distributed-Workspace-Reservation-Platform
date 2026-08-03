/**
 * @file kafka_outbox_message_producer_test.cpp
 * @brief Verifies acknowledged Outbox publication against live Kafka.
 */
#include "haven/infrastructure/messaging/kafka/kafka_outbox_message_producer.hpp"

#include "haven/application/outbox/message_publish_error.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <librdkafka/rdkafka.h>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace haven::infrastructure::messaging::kafka {
namespace {
std::optional<std::string> environment(const char* name) {
    const auto* value = std::getenv(name);
    return value && *value ? std::optional<std::string>{value} : std::nullopt;
}
std::string unique() {
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}
KafkaProducerConfiguration configuration() {
    return {
        .enabled = true,
        .brokers = *environment("HVN_KAFKA_BROKERS"),
        .reservation_events_topic =
            environment("HVN_KAFKA_RESERVATION_EVENTS_TOPIC").value_or("haven.reservation.events"),
        .client_id = "haven-integration-" + unique(),
        .acknowledgement_timeout = std::chrono::seconds{5},
        .delivery_timeout = std::chrono::seconds{10}};
}
haven::application::outbox::OutboxMessage message(std::string suffix,
                                                  std::string organization = "org") {
    return {
        .event_id = haven::domain::EventId{"event-" + suffix},
        .organization_id = haven::domain::OrganizationId{std::move(organization)},
        .aggregate_id = haven::domain::ReservationId{"reservation-" + suffix},
        .aggregate_type = "Reservation",
        .event_type = "ReservationCreated",
        .occurred_at = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}},
        .schema_version = 1,
        .serialized_envelope = "{\"fixture\":\"" + suffix + "\"}",
        .attempt_count = 1};
}

struct ConsumedRecord {
    std::string key;
    std::string payload;
    std::map<std::string, std::string> headers;
};

class Consumer final {
public:
    explicit Consumer(const KafkaProducerConfiguration& settings) {
        auto* configuration = rd_kafka_conf_new();
        char error[512]{};
        auto set = [&](const char* name, const std::string& value) {
            if (rd_kafka_conf_set(configuration, name, value.c_str(), error, sizeof(error)) !=
                RD_KAFKA_CONF_OK)
                throw std::runtime_error{error};
        };
        set("bootstrap.servers", settings.brokers);
        set("group.id", "haven-integration-consumer-" + unique());
        set("auto.offset.reset", "earliest");
        set("enable.auto.commit", "false");
        consumer_.reset(rd_kafka_new(RD_KAFKA_CONSUMER, configuration, error, sizeof(error)));
        if (!consumer_)
            throw std::runtime_error{error};
        rd_kafka_poll_set_consumer(consumer_.get());
        auto* topics = rd_kafka_topic_partition_list_new(1);
        rd_kafka_topic_partition_list_add(topics, settings.reservation_events_topic.c_str(), -1);
        const auto subscribe_error = rd_kafka_subscribe(consumer_.get(), topics);
        rd_kafka_topic_partition_list_destroy(topics);
        if (subscribe_error != RD_KAFKA_RESP_ERR_NO_ERROR)
            throw std::runtime_error{rd_kafka_err2str(subscribe_error)};
    }

    std::vector<ConsumedRecord> receive(const std::vector<std::string>& payloads) {
        auto found = std::vector<ConsumedRecord>{};
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (found.size() < payloads.size() && std::chrono::steady_clock::now() < deadline) {
            std::unique_ptr<rd_kafka_message_t, decltype(&rd_kafka_message_destroy)> record{
                rd_kafka_consumer_poll(consumer_.get(), 200), rd_kafka_message_destroy};
            if (!record || record->err)
                continue;
            const auto payload =
                std::string{static_cast<const char*>(record->payload), record->len};
            if (std::find(payloads.begin(), payloads.end(), payload) == payloads.end())
                continue;
            auto converted = ConsumedRecord{
                .key = std::string{static_cast<const char*>(record->key), record->key_len},
                .payload = payload};
            rd_kafka_headers_t* headers{};
            if (rd_kafka_message_headers(record.get(), &headers) == RD_KAFKA_RESP_ERR_NO_ERROR) {
                for (std::size_t index = 0;; ++index) {
                    const char* name{};
                    const void* value{};
                    std::size_t size{};
                    if (rd_kafka_header_get_all(headers, index, &name, &value, &size) !=
                        RD_KAFKA_RESP_ERR_NO_ERROR)
                        break;
                    converted.headers.emplace(name,
                                              std::string{static_cast<const char*>(value), size});
                }
            }
            found.push_back(std::move(converted));
        }
        return found;
    }

    ~Consumer() {
        if (consumer_)
            static_cast<void>(rd_kafka_consumer_close(consumer_.get()));
    }

private:
    struct Deleter {
        void operator()(rd_kafka_t* value) const noexcept {
            rd_kafka_destroy(value);
        }
    };
    std::unique_ptr<rd_kafka_t, Deleter> consumer_;
};

class KafkaOutboxMessageProducerIntegrationTest : public testing::Test {
protected:
    void SetUp() override {
        if (!environment("HVN_KAFKA_BROKERS"))
            GTEST_SKIP() << "Set HVN_KAFKA_BROKERS for live Kafka tests";
    }
};
}  // namespace

TEST_F(KafkaOutboxMessageProducerIntegrationTest, PublishesExactRecordAndRequiredHeaders) {
    const auto settings = configuration();
    auto consumer = Consumer{settings};
    auto producer = KafkaOutboxMessageProducer{settings};
    const auto source = message(unique());
    producer.publish(source);
    const auto records = consumer.receive({source.serialized_envelope});
    ASSERT_EQ(records.size(), 1U);
    EXPECT_EQ(records[0].key, source.organization_id.value() + "::" + source.aggregate_id.value());
    EXPECT_EQ(records[0].payload, source.serialized_envelope);
    EXPECT_EQ(records[0].headers.size(), 7U);
    EXPECT_EQ(records[0].headers.at("event-id"), source.event_id.value());
    EXPECT_EQ(records[0].headers.at("organization-id"), source.organization_id.value());
    EXPECT_EQ(records[0].headers.at("aggregate-id"), source.aggregate_id.value());
    EXPECT_EQ(records[0].headers.at("aggregate-type"), source.aggregate_type);
    EXPECT_EQ(records[0].headers.at("event-type"), source.event_type);
    EXPECT_EQ(records[0].headers.at("schema-version"), "1");
    EXPECT_EQ(records[0].headers.at("occurred-at"), "2027-01-15T08:00:00.000000000Z");
}

TEST_F(KafkaOutboxMessageProducerIntegrationTest, PreservesSameAggregateOrderAndTenantKeys) {
    const auto settings = configuration();
    auto consumer = Consumer{settings};
    auto producer = KafkaOutboxMessageProducer{settings};
    const auto suffix = unique();
    auto first = message(suffix);
    auto second = first;
    second.event_id = haven::domain::EventId{"second-" + suffix};
    second.serialized_envelope = "{\"fixture\":\"second-" + suffix + "\"}";
    producer.publish(first);
    producer.publish(second);
    const auto records = consumer.receive({first.serialized_envelope, second.serialized_envelope});
    ASSERT_EQ(records.size(), 2U);
    EXPECT_EQ(records[0].payload, first.serialized_envelope);
    EXPECT_EQ(records[1].payload, second.serialized_envelope);
    EXPECT_EQ(records[0].key, records[1].key);
    auto other_tenant = first;
    other_tenant.organization_id = haven::domain::OrganizationId{"other"};
    other_tenant.serialized_envelope = "{\"fixture\":\"other-" + suffix + "\"}";
    producer.publish(other_tenant);
    const auto isolated = consumer.receive({other_tenant.serialized_envelope});
    ASSERT_EQ(isolated.size(), 1U);
    EXPECT_NE(isolated[0].key, records[0].key);
}

TEST_F(KafkaOutboxMessageProducerIntegrationTest, UnavailableBrokerFailsWithinBound) {
    auto settings = configuration();
    settings.brokers = "127.0.0.1:1";
    settings.acknowledgement_timeout = std::chrono::milliseconds{500};
    settings.delivery_timeout = std::chrono::milliseconds{500};
    auto producer = KafkaOutboxMessageProducer{settings};
    const auto start = std::chrono::steady_clock::now();
    try {
        producer.publish(message(unique()));
        FAIL() << "Expected broker failure";
    } catch (const haven::application::outbox::MessagePublishError& error) {
        EXPECT_TRUE(error.code() == haven::application::outbox::MessagePublishErrorCode::Timeout ||
                    error.code() ==
                        haven::application::outbox::MessagePublishErrorCode::Unavailable);
    }
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds{3});
}

TEST_F(KafkaOutboxMessageProducerIntegrationTest, EmptyEnvelopeFailsLocally) {
    auto producer = KafkaOutboxMessageProducer{configuration()};
    auto source = message(unique());
    source.serialized_envelope.clear();
    EXPECT_THROW(producer.publish(source), haven::application::outbox::MessagePublishError);
}

}  // namespace haven::infrastructure::messaging::kafka
