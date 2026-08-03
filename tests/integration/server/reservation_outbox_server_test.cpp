/**
 * @file reservation_outbox_server_test.cpp
 * @brief Verifies the real Reservation HTTP-to-Couchbase-to-Kafka server flow.
 */
#include "haven/application/idempotency/idempotency_scope.hpp"
#include "haven/domain/value_objects/idempotency_key.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_message_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/query_options.hxx>
#include <couchbase/query_scan_consistency.hxx>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <librdkafka/rdkafka.h>
#include <map>
#include <memory>
#include <optional>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <trantor/net/EventLoopThread.h>
#include <unistd.h>
#include <utility>
#include <vector>

extern char** environ;

namespace {
namespace cb = haven::infrastructure::persistence::couchbase;
using namespace std::chrono_literals;

std::optional<std::string> environment(const char* name) {
    const auto* value = std::getenv(name);
    return value && *value ? std::optional<std::string>{value} : std::nullopt;
}

std::string unique() {
    static unsigned sequence{};
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
           std::to_string(++sequence);
}

struct HttpResult {
    drogon::ReqResult result;
    drogon::HttpResponsePtr response;
};

HttpResult request(const std::uint16_t port,
                   const drogon::HttpMethod method,
                   const std::string& path,
                   const std::optional<Json::Value>& body = std::nullopt,
                   const std::map<std::string, std::string>& headers = {}) {
    static auto loop_thread = [] {
        auto thread = std::make_unique<trantor::EventLoopThread>("phase12-http-client");
        thread->run();
        return thread;
    }();
    const auto client =
        drogon::HttpClient::newHttpClient("127.0.0.1", port, false, loop_thread->getLoop());
    const auto outgoing = body ? drogon::HttpRequest::newHttpJsonRequest(*body)
                               : drogon::HttpRequest::newHttpRequest();
    outgoing->setMethod(method);
    outgoing->setPath(path);
    for (const auto& [name, value] : headers)
        outgoing->addHeader(name, value);
    const auto [result, response] = client->sendRequest(outgoing, 5.0);
    return {result, response};
}

class ServerProcess final {
public:
    ServerProcess(const std::uint16_t port,
                  const bool kafka_enabled,
                  std::string brokers,
                  const std::chrono::milliseconds poll_interval = 50ms,
                  const std::chrono::milliseconds kafka_timeout = 500ms)
        : port_(port), log_path_("/tmp/haven-phase12-" + unique() + ".log") {
        auto overrides = std::map<std::string, std::string>{
            {"HVN_HTTP_ADDRESS", "127.0.0.1"},
            {"HVN_HTTP_PORT", std::to_string(port)},
            {"HVN_HTTP_THREADS", "1"},
            {"HVN_REDIS_ENABLED", "false"},
            {"HVN_KAFKA_ENABLED", kafka_enabled ? "true" : "false"},
            {"HVN_KAFKA_BROKERS", std::move(brokers)},
            {"HVN_KAFKA_ACK_TIMEOUT_MS", std::to_string(kafka_timeout.count())},
            {"HVN_KAFKA_DELIVERY_TIMEOUT_MS", std::to_string(kafka_timeout.count())},
            {"HVN_OUTBOX_PUBLISHER_POLL_INTERVAL_MS", std::to_string(poll_interval.count())},
            {"HVN_LOG_LEVEL", "debug"}};
        auto environment_storage = std::vector<std::string>{};
        for (auto item = environ; *item != nullptr; ++item) {
            const auto entry = std::string{*item};
            const auto separator = entry.find('=');
            if (separator == std::string::npos || overrides.contains(entry.substr(0, separator)))
                continue;
            environment_storage.push_back(entry);
        }
        for (const auto& [name, value] : overrides)
            environment_storage.push_back(name + "=" + value);
        auto environment_values = std::vector<char*>{};
        for (auto& value : environment_storage)
            environment_values.push_back(value.data());
        environment_values.push_back(nullptr);

        const auto log = ::open(log_path_.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
        if (log < 0)
            throw std::runtime_error("Unable to open server acceptance log");
        posix_spawn_file_actions_t actions{};
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, log, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, log, STDERR_FILENO);
        char* arguments[]{const_cast<char*>(HAVEN_SERVER_PATH), nullptr};
        const auto spawned = posix_spawn(
            &pid_, HAVEN_SERVER_PATH, &actions, nullptr, arguments, environment_values.data());
        posix_spawn_file_actions_destroy(&actions);
        ::close(log);
        if (spawned != 0)
            throw std::runtime_error("Unable to launch haven-server");
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (std::chrono::steady_clock::now() < deadline) {
            try {
                const auto health = request(port_, drogon::Get, "/health/live");
                if (health.result == drogon::ReqResult::Ok && health.response &&
                    health.response->statusCode() == drogon::k200OK)
                    return;
            } catch (const std::exception&) {}
            std::this_thread::sleep_for(50ms);
        }
        stop();
        throw std::runtime_error("haven-server did not become healthy");
    }

    ~ServerProcess() {
        stop();
    }
    ServerProcess(const ServerProcess&) = delete;
    ServerProcess& operator=(const ServerProcess&) = delete;

    std::chrono::milliseconds stop() noexcept {
        if (pid_ <= 0)
            return 0ms;
        const auto started = std::chrono::steady_clock::now();
        ::kill(pid_, SIGTERM);
        int status{};
        const auto deadline = started + 5s;
        while (std::chrono::steady_clock::now() < deadline) {
            if (::waitpid(pid_, &status, WNOHANG) == pid_) {
                pid_ = -1;
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - started);
            }
            std::this_thread::sleep_for(10ms);
        }
        ::kill(pid_, SIGKILL);
        static_cast<void>(::waitpid(pid_, &status, 0));
        pid_ = -1;
        return 5s;
    }

    const std::string& log_path() const noexcept {
        return log_path_;
    }

private:
    std::uint16_t port_;
    std::string log_path_;
    pid_t pid_{-1};
};

struct ConsumedRecord {
    std::string key;
    std::string payload;
    std::map<std::string, std::string> headers;
};

class Consumer final {
public:
    Consumer(const std::string& brokers, const std::string& topic) {
        auto* configuration = rd_kafka_conf_new();
        char error[512]{};
        auto set = [&](const char* name, const std::string& value) {
            if (rd_kafka_conf_set(configuration, name, value.c_str(), error, sizeof(error)) !=
                RD_KAFKA_CONF_OK)
                throw std::runtime_error{error};
        };
        set("bootstrap.servers", brokers);
        set("group.id", "haven-phase12-acceptance-" + unique());
        set("auto.offset.reset", "earliest");
        set("enable.auto.commit", "false");
        consumer_.reset(rd_kafka_new(RD_KAFKA_CONSUMER, configuration, error, sizeof(error)));
        if (!consumer_)
            throw std::runtime_error{error};
        rd_kafka_poll_set_consumer(consumer_.get());
        auto* topics = rd_kafka_topic_partition_list_new(1);
        rd_kafka_topic_partition_list_add(topics, topic.c_str(), -1);
        const auto subscribed = rd_kafka_subscribe(consumer_.get(), topics);
        rd_kafka_topic_partition_list_destroy(topics);
        if (subscribed != RD_KAFKA_RESP_ERR_NO_ERROR)
            throw std::runtime_error{rd_kafka_err2str(subscribed)};
    }
    ~Consumer() {
        if (consumer_)
            static_cast<void>(rd_kafka_consumer_close(consumer_.get()));
    }

    std::vector<ConsumedRecord> receive(const std::string& organization,
                                        const std::string& reservation,
                                        const std::size_t expected,
                                        const std::chrono::seconds timeout = 10s) {
        auto found = std::vector<ConsumedRecord>{};
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (found.size() < expected && std::chrono::steady_clock::now() < deadline) {
            std::unique_ptr<rd_kafka_message_t, decltype(&rd_kafka_message_destroy)> message{
                rd_kafka_consumer_poll(consumer_.get(), 200), rd_kafka_message_destroy};
            if (!message || message->err)
                continue;
            auto converted = ConsumedRecord{
                .key = std::string{static_cast<const char*>(message->key), message->key_len},
                .payload = std::string{static_cast<const char*>(message->payload), message->len}};
            rd_kafka_headers_t* headers{};
            if (rd_kafka_message_headers(message.get(), &headers) != RD_KAFKA_RESP_ERR_NO_ERROR)
                continue;
            for (std::size_t index = 0;; ++index) {
                const char* name{};
                const void* value{};
                std::size_t size{};
                if (rd_kafka_header_get_all(headers, index, &name, &value, &size) !=
                    RD_KAFKA_RESP_ERR_NO_ERROR)
                    break;
                converted.headers.emplace(name, std::string{static_cast<const char*>(value), size});
            }
            if (converted.headers["organization-id"] == organization &&
                converted.headers["aggregate-id"] == reservation)
                found.push_back(std::move(converted));
        }
        return found;
    }

private:
    struct Deleter {
        void operator()(rd_kafka_t* value) const noexcept {
            rd_kafka_destroy(value);
        }
    };
    std::unique_ptr<rd_kafka_t, Deleter> consumer_;
};

class ReservationOutboxServerIntegrationTest : public testing::Test {
protected:
    void SetUp() override {
        const auto connection_string = environment("HVN_COUCHBASE_CONNECTION_STRING");
        const auto username = environment("HVN_COUCHBASE_USERNAME");
        const auto password = environment("HVN_COUCHBASE_PASSWORD");
        bucket = environment("HVN_COUCHBASE_BUCKET").value_or("");
        scope = environment("HVN_COUCHBASE_SCOPE").value_or("");
        brokers = environment("HVN_KAFKA_BROKERS").value_or("");
        topic =
            environment("HVN_KAFKA_RESERVATION_EVENTS_TOPIC").value_or("haven.reservation.events");
        if (!connection_string || !username || !password || bucket.empty() || scope.empty() ||
            brokers.empty())
            GTEST_SKIP() << "Set HVN_COUCHBASE_* and HVN_KAFKA_BROKERS";
        connection = std::make_shared<cb::CouchbaseConnection>(
            cb::CouchbaseConfiguration{*connection_string, *username, *password, bucket, scope});
        suffix = unique();
        organization = "phase12-org-" + suffix;
        creator = "phase12-user-" + suffix;
        confirmed_resource = "phase12-confirmed-" + suffix;
        approval_resource = "phase12-approval-" + suffix;
        store_resource(confirmed_resource, false);
        store_resource(approval_resource, true);
    }

    void TearDown() override {
        if (!connection)
            return;
        for (const auto& [collection, key] : cleanup) {
            auto [error, ignored] = connection->collection(collection).remove(key).get();
            static_cast<void>(ignored);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << "Cleanup failed for " << key << ": " << error.ec().message();
            auto [get_error, removed] = connection->collection(collection).get(key).get();
            static_cast<void>(removed);
            EXPECT_EQ(get_error.ec(), ::couchbase::errc::key_value::document_not_found)
                << "Fixture remains after cleanup: " << key;
        }
    }

    void store_resource(const std::string& resource, const bool approval) {
        const auto organization_id = haven::domain::OrganizationId{organization};
        const auto resource_id = haven::domain::ResourceId{resource};
        const auto key = cb::resource_document_key(organization_id, resource_id);
        const auto document = cb::ResourceDocument{.schema_version = 1,
                                                   .resource_id = resource,
                                                   .organization_id = organization,
                                                   .name = "Phase 12 fixture",
                                                   .description = "",
                                                   .resource_type = "MEETING_ROOM",
                                                   .status = "ACTIVE",
                                                   .requires_approval = approval,
                                                   .version = 1};
        auto [error, ignored] = connection->collection(cb::CouchbaseCollections::resources)
                                    .insert(key, cb::resource_document_to_json(document))
                                    .get();
        static_cast<void>(ignored);
        ASSERT_FALSE(error) << error.ec().message();
        cleanup.emplace_back(cb::CouchbaseCollections::resources, key);
    }

    HttpResult create(const std::uint16_t port,
                      const std::string& idempotency_key,
                      const std::string& resource,
                      const std::string& purpose = "Phase 12 acceptance") {
        auto body = Json::Value{Json::objectValue};
        body["resourceId"] = resource;
        body["startTime"] = "2035-01-15T10:00:00.000000000Z";
        body["endTime"] = "2035-01-15T11:00:00.000000000Z";
        body["purpose"] = purpose;
        return request(port,
                       drogon::Post,
                       "/api/v1/reservations",
                       body,
                       {{"Idempotency-Key", idempotency_key},
                        {"X-Haven-Organization-Id", organization},
                        {"X-Haven-User-Id", creator},
                        {"X-Request-Id", "phase12-" + suffix}});
    }

    std::vector<cb::OutboxDocument> outbox(const std::string& reservation) {
        auto options = ::couchbase::query_options{};
        options.scan_consistency(::couchbase::query_scan_consistency::request_plus)
            .named_parameters(std::make_pair("organization", organization),
                              std::make_pair("aggregate", reservation));
        const auto statement =
            "SELECT RAW o FROM `outbox` o WHERE o.documentType = \"outbox\" "
            "AND o.organizationId = $organization AND o.aggregateId = $aggregate "
            "ORDER BY o.occurredAt, o.eventId";
        auto [error, result] = connection->scope().query(statement, options).get();
        EXPECT_FALSE(error) << error.ec().message();
        auto documents = std::vector<cb::OutboxDocument>{};
        for (const auto& row : result.rows_as())
            documents.push_back(cb::outbox_document_from_json(row));
        return documents;
    }

    std::vector<cb::OutboxDocument> wait_outbox(const std::string& reservation,
                                                const cb::OutboxStatus status,
                                                const std::chrono::seconds timeout = 12s) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            auto documents = outbox(reservation);
            if (documents.size() == 2 &&
                std::all_of(documents.begin(), documents.end(), [status](const auto& document) {
                    return document.status == status;
                }))
                return documents;
            std::this_thread::sleep_for(50ms);
        }
        return outbox(reservation);
    }

    void track_created(const std::string& key,
                       const std::string& reservation,
                       const std::vector<cb::OutboxDocument>& documents) {
        cleanup.emplace_back(
            cb::CouchbaseCollections::reservations,
            cb::reservation_document_key(haven::domain::OrganizationId{organization},
                                         haven::domain::ReservationId{reservation}));
        for (const auto& document : documents)
            cleanup.emplace_back(
                cb::CouchbaseCollections::outbox,
                cb::outbox_document_key(document.organization_id, document.event_id));
        const auto idempotency_scope = haven::application::idempotency::IdempotencyScope{
            haven::domain::OrganizationId{organization},
            haven::domain::UserId{creator},
            haven::application::idempotency::IdempotencyOperation::CreateReservation,
            haven::domain::IdempotencyKey{key}};
        cleanup.emplace_back(cb::CouchbaseCollections::idempotency,
                             cb::idempotency_document_key(idempotency_scope));
    }

    std::uint16_t port() const {
        static std::uint16_t next = static_cast<std::uint16_t>(19080 + (::getpid() % 500));
        return next++;
    }

    std::shared_ptr<cb::CouchbaseConnection> connection;
    std::string bucket;
    std::string scope;
    std::string brokers;
    std::string topic;
    std::string suffix;
    std::string organization;
    std::string creator;
    std::string confirmed_resource;
    std::string approval_resource;
    std::vector<std::pair<std::string_view, std::string>> cleanup;
};

TEST_F(ReservationOutboxServerIntegrationTest,
       ConfirmedPublicationReplayAndMismatchUseStableRecords) {
    const auto server_port = port();
    auto consumer = Consumer{brokers, topic};
    auto server = ServerProcess{server_port, true, brokers};
    const auto key = "confirmed-" + suffix;
    const auto original = create(server_port, key, confirmed_resource);
    ASSERT_EQ(original.result, drogon::ReqResult::Ok);
    ASSERT_TRUE(original.response);
    ASSERT_EQ(original.response->statusCode(), drogon::k201Created);
    const auto original_body = original.response->getJsonObject();
    ASSERT_TRUE(original_body);
    EXPECT_EQ((*original_body)["status"].asString(), "CONFIRMED");
    EXPECT_EQ(original_body->size(), 6U);
    const auto reservation = (*original_body)["reservationId"].asString();
    const auto documents = wait_outbox(reservation, cb::OutboxStatus::Published);
    ASSERT_EQ(documents.size(), 2U);
    track_created(key, reservation, documents);
    EXPECT_EQ(documents[0].event_type, cb::kReservationCreatedEventType);
    EXPECT_EQ(documents[1].event_type, cb::kReservationConfirmedEventType);
    EXPECT_NE(documents[0].event_id, documents[1].event_id);
    for (const auto& document : documents) {
        EXPECT_TRUE(document.published_at.has_value());
        EXPECT_GE(document.attempt_count, 1U);
    }
    auto [reservation_error, reservation_result] =
        connection->collection(cb::CouchbaseCollections::reservations)
            .get(cb::reservation_document_key(haven::domain::OrganizationId{organization},
                                              haven::domain::ReservationId{reservation}))
            .get();
    ASSERT_FALSE(reservation_error);
    const auto reservation_cas = reservation_result.cas();
    EXPECT_EQ(cb::reservation_document_from_json(reservation_result.content_as<tao::json::value>())
                  .version,
              1U);

    const auto records = consumer.receive(organization, reservation, 2);
    ASSERT_EQ(records.size(), 2U);
    for (std::size_t index = 0; index < records.size(); ++index) {
        const auto expected = cb::to_outbox_message(documents[index]);
        EXPECT_EQ(records[index].payload, expected.serialized_envelope);
        EXPECT_EQ(records[index].key, organization + "::" + reservation);
        EXPECT_EQ(records[index].headers.size(), 7U);
        EXPECT_EQ(records[index].headers.at("event-id"), documents[index].event_id.value());
        EXPECT_EQ(records[index].headers.at("event-type"), documents[index].event_type);
        EXPECT_EQ(records[index].headers.at("organization-id"), organization);
        EXPECT_EQ(records[index].headers.at("aggregate-id"), reservation);
        EXPECT_EQ(records[index].headers.at("aggregate-type"), "Reservation");
        EXPECT_EQ(records[index].headers.at("schema-version"), "1");
        EXPECT_FALSE(records[index].headers.at("occurred-at").empty());
    }

    const auto replay = create(server_port, key, confirmed_resource);
    ASSERT_EQ(replay.response->statusCode(), drogon::k201Created);
    EXPECT_EQ(*replay.response->getJsonObject(), *original_body);
    const auto after_replay = wait_outbox(reservation, cb::OutboxStatus::Published);
    EXPECT_EQ(after_replay, documents);
    EXPECT_TRUE(consumer.receive(organization, reservation, 1, 1s).empty());
    const auto mismatch = create(server_port, key, confirmed_resource, "different");
    ASSERT_EQ(mismatch.response->statusCode(), drogon::k409Conflict);
    EXPECT_EQ((*mismatch.response->getJsonObject())["code"].asString(), "IDEMPOTENCY_KEY_MISMATCH");
    auto [after_error, after_result] =
        connection->collection(cb::CouchbaseCollections::reservations)
            .get(cb::reservation_document_key(haven::domain::OrganizationId{organization},
                                              haven::domain::ReservationId{reservation}))
            .get();
    ASSERT_FALSE(after_error);
    EXPECT_EQ(after_result.cas(), reservation_cas);
    EXPECT_EQ(outbox(reservation), documents);
}

TEST_F(ReservationOutboxServerIntegrationTest,
       PendingApprovalPublishesOnlyCreatedAndApprovalRequested) {
    const auto server_port = port();
    auto consumer = Consumer{brokers, topic};
    auto server = ServerProcess{server_port, true, brokers};
    const auto key = "approval-" + suffix;
    const auto created = create(server_port, key, approval_resource);
    ASSERT_EQ(created.response->statusCode(), drogon::k201Created);
    const auto body = created.response->getJsonObject();
    ASSERT_TRUE(body);
    EXPECT_EQ((*body)["status"].asString(), "PENDING_APPROVAL");
    const auto reservation = (*body)["reservationId"].asString();
    const auto documents = wait_outbox(reservation, cb::OutboxStatus::Published);
    ASSERT_EQ(documents.size(), 2U);
    track_created(key, reservation, documents);
    EXPECT_EQ(documents[0].event_type, cb::kReservationCreatedEventType);
    EXPECT_EQ(documents[1].event_type, cb::kReservationApprovalRequestedEventType);
    const auto records = consumer.receive(organization, reservation, 2);
    ASSERT_EQ(records.size(), 2U);
    EXPECT_EQ(records[0].headers.at("event-type"), cb::kReservationCreatedEventType);
    EXPECT_EQ(records[1].headers.at("event-type"), cb::kReservationApprovalRequestedEventType);
}

TEST_F(ReservationOutboxServerIntegrationTest,
       KafkaDisabledPersistsPendingAndRestartPublishesStableEvents) {
    const auto server_port = port();
    const auto key = "restart-" + suffix;
    auto disabled = ServerProcess{server_port, false, "127.0.0.1:1", 60s};
    const auto created = create(server_port, key, confirmed_resource);
    ASSERT_EQ(created.response->statusCode(), drogon::k201Created);
    const auto reservation = (*created.response->getJsonObject())["reservationId"].asString();
    const auto pending = wait_outbox(reservation, cb::OutboxStatus::Pending, 2s);
    ASSERT_EQ(pending.size(), 2U);
    track_created(key, reservation, pending);
    EXPECT_EQ(pending[0].attempt_count, 0U);
    EXPECT_EQ(pending[1].attempt_count, 0U);
    EXPECT_LT(disabled.stop(), 1s);

    auto consumer = Consumer{brokers, topic};
    auto restarted = ServerProcess{server_port, true, brokers};
    const auto published = wait_outbox(reservation, cb::OutboxStatus::Published);
    ASSERT_EQ(published.size(), 2U);
    EXPECT_EQ(published[0].event_id, pending[0].event_id);
    EXPECT_EQ(published[1].event_id, pending[1].event_id);
    EXPECT_EQ(consumer.receive(organization, reservation, 2).size(), 2U);
}

TEST_F(ReservationOutboxServerIntegrationTest, UnavailableKafkaRetriesPendingAndShutdownIsBounded) {
    const auto server_port = port();
    const auto key = "unavailable-" + suffix;
    auto unavailable = ServerProcess{server_port, true, "127.0.0.1:1", 50ms, 500ms};
    const auto created = create(server_port, key, confirmed_resource);
    ASSERT_EQ(created.response->statusCode(), drogon::k201Created);
    const auto reservation = (*created.response->getJsonObject())["reservationId"].asString();
    auto pending = std::vector<cb::OutboxDocument>{};
    const auto attempted_deadline = std::chrono::steady_clock::now() + 4s;
    do {
        pending = outbox(reservation);
        if (pending.size() == 2 &&
            std::all_of(pending.begin(), pending.end(), [](const auto& document) {
                return document.status == cb::OutboxStatus::Pending && document.attempt_count >= 1;
            }))
            break;
        std::this_thread::sleep_for(50ms);
    } while (std::chrono::steady_clock::now() < attempted_deadline);
    ASSERT_EQ(pending.size(), 2U);
    track_created(key, reservation, pending);
    EXPECT_GE(pending[0].attempt_count, 1U);
    EXPECT_GE(pending[1].attempt_count, 1U);
    EXPECT_LT(unavailable.stop(), 2s);

    auto consumer = Consumer{brokers, topic};
    auto recovered = ServerProcess{server_port, true, brokers};
    const auto published = wait_outbox(reservation, cb::OutboxStatus::Published);
    ASSERT_EQ(published.size(), 2U);
    EXPECT_EQ(published[0].event_id, pending[0].event_id);
    EXPECT_EQ(published[1].event_id, pending[1].event_id);
    EXPECT_GT(published[0].attempt_count, pending[0].attempt_count);
    EXPECT_GT(published[1].attempt_count, pending[1].attempt_count);
    EXPECT_EQ(consumer.receive(organization, reservation, 2).size(), 2U);
}

}  // namespace
