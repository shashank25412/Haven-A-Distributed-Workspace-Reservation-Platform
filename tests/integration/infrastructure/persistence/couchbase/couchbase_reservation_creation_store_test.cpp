/** @file couchbase_reservation_creation_store_test.cpp */
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_store.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace {
namespace cb = haven::infrastructure::persistence::couchbase;
using Event = haven::domain::ReservationDomainEvent;

[[nodiscard]] std::optional<std::string> env(const char* name) {
    const auto* value = std::getenv(name);
    return value && *value ? std::optional<std::string>{value} : std::nullopt;
}
[[nodiscard]] std::optional<cb::CouchbaseConfiguration> config() {
    const auto connection = env("HVN_COUCHBASE_CONNECTION_STRING");
    const auto username = env("HVN_COUCHBASE_USERNAME");
    const auto password = env("HVN_COUCHBASE_PASSWORD");
    const auto bucket = env("HVN_COUCHBASE_BUCKET");
    const auto scope = env("HVN_COUCHBASE_SCOPE");
    if (!connection || !username || !password || !bucket || !scope)
        return std::nullopt;
    return cb::CouchbaseConfiguration{*connection, *username, *password, *bucket, *scope};
}
[[nodiscard]] std::string suffix() {
    static unsigned sequence{};
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
           std::to_string(++sequence);
}
[[nodiscard]] auto at(long seconds) {
    return haven::domain::Reservation::TimePoint{std::chrono::seconds{1'800'000'000 + seconds}};
}
[[nodiscard]] const haven::domain::EventId& id(const Event& event) {
    return std::visit(
        [](const auto& value) -> const haven::domain::EventId& { return value.event_id(); }, event);
}

class CouchbaseReservationCreationStoreIntegrationTest : public testing::Test {
protected:
    void SetUp() override {
        const auto configuration = config();
        if (!configuration)
            GTEST_SKIP() << "Set all HVN_COUCHBASE_* variables";
        connection = std::make_shared<cb::CouchbaseConnection>(*configuration);
        store = std::make_unique<cb::CouchbaseReservationCreationStore>(connection);
    }
    void TearDown() override {
        if (!connection)
            return;
        for (const auto& [collection_name, key] : cleanup) {
            auto [error, ignored] = connection->collection(collection_name).remove(key).get();
            static_cast<void>(ignored);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << error.ec().message();
        }
    }
    [[nodiscard]] haven::domain::Reservation confirmed(const std::string& unique) {
        return haven::domain::Reservation::create_confirmed(
            haven::domain::OrganizationId{"org-" + unique},
            haven::domain::ReservationId{"reservation-" + unique},
            haven::domain::ResourceId{"resource-" + unique},
            haven::domain::UserId{"user-" + unique},
            haven::domain::TimeInterval{at(10), at(20)},
            haven::domain::Purpose{"Work"},
            haven::domain::ReservationKind::Standard,
            haven::domain::EventId{"created-" + unique},
            haven::domain::EventId{"confirmed-" + unique},
            at(1));
    }
    void track(const haven::domain::Reservation& reservation, const std::vector<Event>& events) {
        cleanup.emplace_back(cb::CouchbaseCollections::reservations,
                             cb::reservation_document_key(reservation.organization_id(),
                                                          reservation.reservation_id()));
        for (const auto& event : events)
            cleanup.emplace_back(cb::CouchbaseCollections::outbox,
                                 cb::outbox_document_key(reservation.organization_id(), id(event)));
    }
    [[nodiscard]] bool exists(std::string_view collection, const std::string& key) {
        auto [error, ignored] = connection->collection(collection).get(key).get();
        static_cast<void>(ignored);
        return !error;
    }
    [[nodiscard]] tao::json::value json(std::string_view collection, const std::string& key) {
        auto [error, result] = connection->collection(collection).get(key).get();
        EXPECT_FALSE(error) << error.ec().message();
        return result.content_as<tao::json::value>();
    }
    std::shared_ptr<cb::CouchbaseConnection> connection;
    std::unique_ptr<cb::CouchbaseReservationCreationStore> store;
    std::vector<std::pair<std::string_view, std::string>> cleanup;
};

TEST_F(CouchbaseReservationCreationStoreIntegrationTest,
       PersistsReservationAndTwoMappedOutboxDocuments) {
    auto reservation = confirmed(suffix());
    auto events = reservation.release_domain_events();
    track(reservation, events);
    static_cast<void>(store->persist(reservation.organization_id(), reservation, events));
    EXPECT_TRUE(exists(cb::CouchbaseCollections::reservations, cleanup.front().second));
    ASSERT_EQ(events.size(), 2U);
    for (const auto& event : events) {
        const auto actual = cb::outbox_document_from_json(
            json(cb::CouchbaseCollections::outbox,
                 cb::outbox_document_key(reservation.organization_id(), id(event))));
        EXPECT_EQ(actual,
                  cb::to_outbox_document(
                      reservation.organization_id(), reservation.reservation_id(), event));
    }
}

TEST_F(CouchbaseReservationCreationStoreIntegrationTest,
       PersistsPendingCreatedAndApprovalRequestedEvents) {
    const auto unique = suffix();
    auto reservation = haven::domain::Reservation::create_pending_approval(
        haven::domain::OrganizationId{"org-" + unique},
        haven::domain::ReservationId{"reservation-" + unique},
        haven::domain::ResourceId{"resource-" + unique},
        haven::domain::UserId{"user-" + unique},
        haven::domain::TimeInterval{at(10), at(20)},
        haven::domain::Purpose{"Work"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"created-" + unique},
        haven::domain::EventId{"requested-" + unique},
        at(1));
    auto events = reservation.release_domain_events();
    track(reservation, events);
    static_cast<void>(store->persist(reservation.organization_id(), reservation, events));
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(
        cb::outbox_document_from_json(json(cb::CouchbaseCollections::outbox, cleanup[1].second))
            .event_type,
        cb::kReservationCreatedEventType);
    EXPECT_EQ(
        cb::outbox_document_from_json(json(cb::CouchbaseCollections::outbox, cleanup[2].second))
            .event_type,
        cb::kReservationApprovalRequestedEventType);
}

TEST_F(CouchbaseReservationCreationStoreIntegrationTest, DuplicateReservationMapsToAlreadyExists) {
    auto reservation = confirmed(suffix());
    auto events = reservation.release_domain_events();
    track(reservation, events);
    static_cast<void>(store->persist(reservation.organization_id(), reservation, events));
    try {
        static_cast<void>(store->persist(reservation.organization_id(), reservation, events));
        FAIL() << "Expected duplicate failure";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::AlreadyExists);
    }
    EXPECT_TRUE(exists(cb::CouchbaseCollections::outbox, cleanup[1].second));
    EXPECT_TRUE(exists(cb::CouchbaseCollections::outbox, cleanup[2].second));
}

TEST_F(CouchbaseReservationCreationStoreIntegrationTest,
       DuplicateOutboxKeyRollsBackBothCollections) {
    auto reservation = confirmed(suffix());
    auto released = reservation.release_domain_events();
    auto duplicate = std::vector<Event>{released.front(), released.front()};
    track(reservation, duplicate);
    EXPECT_THROW(
        static_cast<void>(store->persist(reservation.organization_id(), reservation, duplicate)),
        haven::application::RepositoryError);
    EXPECT_FALSE(exists(cb::CouchbaseCollections::reservations, cleanup.front().second));
    EXPECT_FALSE(exists(cb::CouchbaseCollections::outbox, cleanup[1].second));
}
}  // namespace
