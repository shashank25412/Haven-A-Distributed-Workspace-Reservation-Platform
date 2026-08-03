/**
 * @file outbox_pipeline_test.cpp
 * @brief Verifies Reservation-to-Couchbase-to-Kafka Outbox publication.
 */
#include "haven/application/outbox/outbox_publisher.hpp"
#include "haven/application/outbox/system_outbox_publisher_clock.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_outbox_message_producer.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_outbox_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_store.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_message_mapper.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <cstdlib>
#include <librdkafka/rdkafka.h>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
namespace cb = haven::infrastructure::persistence::couchbase;
namespace kafka = haven::infrastructure::messaging::kafka;
using Event = haven::domain::ReservationDomainEvent;

std::optional<std::string> environment(const char* name) {
    const auto* value = std::getenv(name);
    return value && *value ? std::optional<std::string>{value} : std::nullopt;
}
std::string unique() {
    static unsigned sequence{};
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
           std::to_string(++sequence);
}
const haven::domain::EventId& event_id(const Event& event) {
    return std::visit(
        [](const auto& value) -> const haven::domain::EventId& { return value.event_id(); }, event);
}
kafka::KafkaProducerConfiguration kafka_configuration() {
    return {
        .enabled = true,
        .brokers = *environment("HVN_KAFKA_BROKERS"),
        .reservation_events_topic =
            environment("HVN_KAFKA_RESERVATION_EVENTS_TOPIC").value_or("haven.reservation.events"),
        .client_id = "haven-pipeline-" + unique(),
        .acknowledgement_timeout = std::chrono::seconds{5},
        .delivery_timeout = std::chrono::seconds{10}};
}

struct ConsumedRecord {
    std::string key;
    std::string payload;
    std::map<std::string, std::string> headers;
};

class Consumer final {
public:
    explicit Consumer(const kafka::KafkaProducerConfiguration& settings) {
        auto* configuration = rd_kafka_conf_new();
        char error[512]{};
        auto set = [&](const char* name, const std::string& value) {
            if (rd_kafka_conf_set(configuration, name, value.c_str(), error, sizeof(error)) !=
                RD_KAFKA_CONF_OK)
                throw std::runtime_error{error};
        };
        set("bootstrap.servers", settings.brokers);
        set("group.id", "haven-pipeline-consumer-" + unique());
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
    ~Consumer() {
        if (consumer_)
            static_cast<void>(rd_kafka_consumer_close(consumer_.get()));
    }

    std::vector<ConsumedRecord> receive(const std::vector<std::string>& payloads) {
        auto records = std::vector<ConsumedRecord>{};
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (records.size() < payloads.size() && std::chrono::steady_clock::now() < deadline) {
            std::unique_ptr<rd_kafka_message_t, decltype(&rd_kafka_message_destroy)> message{
                rd_kafka_consumer_poll(consumer_.get(), 200), rd_kafka_message_destroy};
            if (!message || message->err)
                continue;
            const auto payload =
                std::string{static_cast<const char*>(message->payload), message->len};
            if (std::find(payloads.begin(), payloads.end(), payload) == payloads.end())
                continue;
            auto record = ConsumedRecord{
                .key = std::string{static_cast<const char*>(message->key), message->key_len},
                .payload = payload};
            rd_kafka_headers_t* headers{};
            if (rd_kafka_message_headers(message.get(), &headers) == RD_KAFKA_RESP_ERR_NO_ERROR) {
                for (std::size_t index = 0;; ++index) {
                    const char* name{};
                    const void* value{};
                    std::size_t size{};
                    if (rd_kafka_header_get_all(headers, index, &name, &value, &size) !=
                        RD_KAFKA_RESP_ERR_NO_ERROR)
                        break;
                    record.headers.emplace(name,
                                           std::string{static_cast<const char*>(value), size});
                }
            }
            records.push_back(std::move(record));
        }
        return records;
    }

private:
    struct Deleter {
        void operator()(rd_kafka_t* value) const noexcept {
            rd_kafka_destroy(value);
        }
    };
    std::unique_ptr<rd_kafka_t, Deleter> consumer_;
};

class OutboxPipelineIntegrationTest : public testing::Test {
protected:
    void SetUp() override {
        const auto connection_string = environment("HVN_COUCHBASE_CONNECTION_STRING");
        const auto username = environment("HVN_COUCHBASE_USERNAME");
        const auto password = environment("HVN_COUCHBASE_PASSWORD");
        const auto bucket = environment("HVN_COUCHBASE_BUCKET");
        const auto scope = environment("HVN_COUCHBASE_SCOPE");
        if (!connection_string || !username || !password || !bucket || !scope ||
            !environment("HVN_KAFKA_BROKERS"))
            GTEST_SKIP() << "Set HVN_COUCHBASE_* and HVN_KAFKA_BROKERS";
        connection = std::make_shared<cb::CouchbaseConnection>(
            cb::CouchbaseConfiguration{*connection_string, *username, *password, *bucket, *scope});
        creation_store = std::make_unique<cb::CouchbaseReservationCreationStore>(connection);
        repository = std::make_unique<cb::CouchbaseOutboxRepository>(connection);
    }
    void TearDown() override {
        if (!connection)
            return;
        for (const auto& [collection, key] : cleanup) {
            auto [error, ignored] = connection->collection(collection).remove(key).get();
            static_cast<void>(ignored);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << error.ec().message();
        }
    }
    haven::domain::Reservation reservation(const std::string& suffix) {
        const auto occurred =
            haven::domain::Reservation::TimePoint{std::chrono::seconds{1'800'000'000}};
        return haven::domain::Reservation::create_confirmed(
            haven::domain::OrganizationId{"org-" + suffix},
            haven::domain::ReservationId{"reservation-" + suffix},
            haven::domain::ResourceId{"resource-" + suffix},
            haven::domain::UserId{"user-" + suffix},
            haven::domain::TimeInterval{occurred + std::chrono::hours{1},
                                        occurred + std::chrono::hours{2}},
            haven::domain::Purpose{"Pipeline"},
            haven::domain::ReservationKind::Standard,
            haven::domain::EventId{"a-created-" + suffix},
            haven::domain::EventId{"b-confirmed-" + suffix},
            occurred);
    }
    void track(const haven::domain::Reservation& reservation, const std::vector<Event>& events) {
        cleanup.emplace_back(cb::CouchbaseCollections::reservations,
                             cb::reservation_document_key(reservation.organization_id(),
                                                          reservation.reservation_id()));
        for (const auto& event : events)
            cleanup.emplace_back(
                cb::CouchbaseCollections::outbox,
                cb::outbox_document_key(reservation.organization_id(), event_id(event)));
    }
    cb::OutboxDocument outbox(const haven::domain::OrganizationId& organization,
                              const haven::domain::EventId& event) {
        auto [error, result] = connection->collection(cb::CouchbaseCollections::outbox)
                                   .get(cb::outbox_document_key(organization, event))
                                   .get();
        EXPECT_FALSE(error) << error.ec().message();
        return cb::outbox_document_from_json(result.content_as<tao::json::value>());
    }
    bool reservation_exists(const haven::domain::Reservation& reservation) {
        auto [error, ignored] = connection->collection(cb::CouchbaseCollections::reservations)
                                    .get(cb::reservation_document_key(reservation.organization_id(),
                                                                      reservation.reservation_id()))
                                    .get();
        static_cast<void>(ignored);
        return !error;
    }

    std::shared_ptr<cb::CouchbaseConnection> connection;
    std::unique_ptr<cb::CouchbaseReservationCreationStore> creation_store;
    std::unique_ptr<cb::CouchbaseOutboxRepository> repository;
    std::vector<std::pair<std::string_view, std::string>> cleanup;
};

TEST_F(OutboxPipelineIntegrationTest, PublishesCreatedReservationEventsAndCompletesOutbox) {
    auto aggregate = reservation(unique());
    auto events = aggregate.release_domain_events();
    track(aggregate, events);
    static_cast<void>(creation_store->persist(aggregate.organization_id(), aggregate, events));
    ASSERT_TRUE(reservation_exists(aggregate));
    ASSERT_EQ(events.size(), 2U);
    auto expected_messages = std::vector<haven::application::outbox::OutboxMessage>{};
    for (const auto& event : events) {
        const auto document = outbox(aggregate.organization_id(), event_id(event));
        EXPECT_EQ(document.status, cb::OutboxStatus::Pending);
        EXPECT_EQ(document.attempt_count, 0U);
        expected_messages.push_back(cb::to_outbox_message(document));
    }

    const auto settings = kafka_configuration();
    auto consumer = Consumer{settings};
    auto producer = kafka::KafkaOutboxMessageProducer{settings};
    auto clock = haven::application::outbox::SystemOutboxPublisherClock{};
    auto publisher = haven::application::outbox::OutboxPublisher{*repository, producer, clock};
    const auto result = publisher.run_once(2);
    EXPECT_EQ(result.candidates_found, 2U);
    EXPECT_EQ(result.claims_acquired, 2U);
    EXPECT_EQ(result.published, 2U);

    const auto records = consumer.receive(
        {expected_messages[0].serialized_envelope, expected_messages[1].serialized_envelope});
    ASSERT_EQ(records.size(), 2U);
    for (std::size_t index = 0; index < records.size(); ++index) {
        EXPECT_EQ(records[index].payload, expected_messages[index].serialized_envelope);
        EXPECT_EQ(records[index].key,
                  aggregate.organization_id().value() + "::" + aggregate.reservation_id().value());
        EXPECT_EQ(records[index].headers.at("event-id"), expected_messages[index].event_id.value());
        EXPECT_EQ(records[index].headers.at("organization-id"),
                  aggregate.organization_id().value());
        EXPECT_EQ(records[index].headers.at("aggregate-id"), aggregate.reservation_id().value());
    }
    EXPECT_EQ(records[0].headers.at("event-type"), cb::kReservationCreatedEventType);
    EXPECT_EQ(records[1].headers.at("event-type"), cb::kReservationConfirmedEventType);
    for (const auto& event : events) {
        const auto document = outbox(aggregate.organization_id(), event_id(event));
        EXPECT_EQ(document.status, cb::OutboxStatus::Published);
        EXPECT_TRUE(document.published_at.has_value());
        EXPECT_EQ(document.attempt_count, 1U);
    }
}

TEST_F(OutboxPipelineIntegrationTest, FailedKafkaReleaseCanBePublishedByLaterExplicitCycle) {
    auto aggregate = reservation(unique());
    auto events = aggregate.release_domain_events();
    track(aggregate, events);
    static_cast<void>(creation_store->persist(aggregate.organization_id(), aggregate, events));
    auto unavailable = kafka_configuration();
    unavailable.brokers = "127.0.0.1:1";
    unavailable.acknowledgement_timeout = std::chrono::milliseconds{300};
    unavailable.delivery_timeout = std::chrono::milliseconds{300};
    auto failed_producer = kafka::KafkaOutboxMessageProducer{unavailable};
    auto clock = haven::application::outbox::SystemOutboxPublisherClock{};
    auto failed_publisher =
        haven::application::outbox::OutboxPublisher{*repository, failed_producer, clock};
    const auto failed = failed_publisher.run_once(2);
    EXPECT_EQ(failed.released_for_retry, 2U);
    ASSERT_TRUE(reservation_exists(aggregate));
    auto expected_payloads = std::vector<std::string>{};
    for (const auto& event : events) {
        const auto document = outbox(aggregate.organization_id(), event_id(event));
        EXPECT_EQ(document.status, cb::OutboxStatus::Pending);
        EXPECT_FALSE(document.published_at.has_value());
        EXPECT_EQ(document.attempt_count, 1U);
        expected_payloads.push_back(cb::to_outbox_message(document).serialized_envelope);
    }

    const auto healthy = kafka_configuration();
    auto consumer = Consumer{healthy};
    auto producer = kafka::KafkaOutboxMessageProducer{healthy};
    auto publisher = haven::application::outbox::OutboxPublisher{*repository, producer, clock};
    const auto recovered = publisher.run_once(2);
    EXPECT_EQ(recovered.published, 2U);
    EXPECT_EQ(consumer.receive(expected_payloads).size(), 2U);
    for (const auto& event : events) {
        const auto document = outbox(aggregate.organization_id(), event_id(event));
        EXPECT_EQ(document.status, cb::OutboxStatus::Published);
        EXPECT_TRUE(document.published_at.has_value());
        EXPECT_EQ(document.attempt_count, 2U);
    }
}

}  // namespace
