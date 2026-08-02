/**
 * @file couchbase_reservation_creation_event_store_test.cpp
 * @brief Tests Couchbase creation-event recovery reads against a live service.
 */
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_event_store.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/events/reservation_confirmed_event.hpp"
#include "haven/domain/events/reservation_created_event.hpp"
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
#include <vector>

namespace {
namespace cb = haven::infrastructure::persistence::couchbase;

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
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

class CouchbaseReservationCreationEventStoreIntegrationTest : public testing::Test {
protected:
    void SetUp() override {
        const auto configuration = config();
        if (!configuration)
            GTEST_SKIP() << "Set all HVN_COUCHBASE_* variables";
        connection = std::make_shared<cb::CouchbaseConnection>(*configuration);
        store = std::make_unique<cb::CouchbaseReservationCreationEventStore>(connection);
    }
    void TearDown() override {
        if (!connection)
            return;
        auto collection = connection->collection(cb::CouchbaseCollections::outbox);
        for (const auto& key : keys) {
            auto [error, ignored] = collection.remove(key).get();
            static_cast<void>(ignored);
            if (error && error.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << error.ec().message();
        }
    }
    [[nodiscard]] cb::OutboxDocument document(const haven::domain::OrganizationId& organization,
                                              const haven::domain::ReservationId& reservation,
                                              const haven::domain::EventId& event_id) {
        const auto now = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}};
        const auto event = haven::domain::ReservationCreatedEvent{
            event_id,
            now,
            organization,
            reservation,
            haven::domain::ResourceId{"resource"},
            haven::domain::UserId{"creator"},
            haven::domain::TimeInterval{now, now + std::chrono::hours{1}},
            haven::domain::ReservationKind::Standard,
            haven::domain::ReservationStatus::Confirmed};
        return cb::to_outbox_document(organization, reservation, event);
    }
    [[nodiscard]] cb::OutboxDocument confirmed_document(
        const haven::domain::OrganizationId& organization,
        const haven::domain::ReservationId& reservation,
        const haven::domain::EventId& event_id) {
        const auto now = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}};
        const auto event = haven::domain::ReservationConfirmedEvent{
            event_id,
            now,
            organization,
            reservation,
            haven::domain::ResourceId{"resource"},
            haven::domain::TimeInterval{now, now + std::chrono::hours{1}},
            std::nullopt};
        return cb::to_outbox_document(organization, reservation, event);
    }
    void insert(const haven::domain::OrganizationId& organization, cb::OutboxDocument document) {
        const auto key = cb::outbox_document_key(organization, document.event_id);
        keys.push_back(key);
        auto [error, ignored] = connection->collection(cb::CouchbaseCollections::outbox)
                                    .insert(key, cb::outbox_document_to_json(document))
                                    .get();
        static_cast<void>(ignored);
        ASSERT_FALSE(error) << error.ec().message();
    }
    std::shared_ptr<cb::CouchbaseConnection> connection;
    std::unique_ptr<cb::CouchbaseReservationCreationEventStore> store;
    std::vector<std::string> keys;
};

TEST_F(CouchbaseReservationCreationEventStoreIntegrationTest,
       ReturnsTrueForAllExpectedEventsIncludingPublished) {
    const auto unique = suffix();
    const auto organization = haven::domain::OrganizationId{"org-" + unique};
    const auto reservation = haven::domain::ReservationId{"reservation-" + unique};
    const auto first = haven::domain::EventId{"first-" + unique};
    const auto second = haven::domain::EventId{"second-" + unique};
    insert(organization, document(organization, reservation, first));
    auto published = confirmed_document(organization, reservation, second);
    published.status = cb::OutboxStatus::Published;
    published.published_at = published.occurred_at + std::chrono::seconds{1};
    insert(organization, std::move(published));
    EXPECT_TRUE(store->contains_all(organization, reservation, {first, second}));
}

TEST_F(CouchbaseReservationCreationEventStoreIntegrationTest, ReturnsFalseWhenSomeOrAllAreAbsent) {
    const auto unique = suffix();
    const auto organization = haven::domain::OrganizationId{"org-" + unique};
    const auto reservation = haven::domain::ReservationId{"reservation-" + unique};
    const auto present = haven::domain::EventId{"present-" + unique};
    const auto absent = haven::domain::EventId{"absent-" + unique};
    insert(organization, document(organization, reservation, present));
    EXPECT_FALSE(store->contains_all(organization, reservation, {present, absent}));
    EXPECT_FALSE(store->contains_all(organization, reservation, {absent}));
}

TEST_F(CouchbaseReservationCreationEventStoreIntegrationTest,
       RejectsAggregateMismatchAndPreservesOrganizationIsolation) {
    const auto unique = suffix();
    const auto organization = haven::domain::OrganizationId{"org-" + unique};
    const auto other_organization = haven::domain::OrganizationId{"other-org-" + unique};
    const auto expected = haven::domain::ReservationId{"expected-" + unique};
    const auto other = haven::domain::ReservationId{"other-" + unique};
    const auto created_id = haven::domain::EventId{"created-" + unique};
    const auto confirmed_id = haven::domain::EventId{"confirmed-" + unique};
    insert(organization, document(organization, expected, created_id));
    insert(organization, confirmed_document(organization, other, confirmed_id));
    EXPECT_THROW(
        static_cast<void>(store->contains_all(organization, expected, {created_id, confirmed_id})),
        haven::application::RepositoryError);
    EXPECT_FALSE(store->contains_all(other_organization, other, {created_id, confirmed_id}));
}
}  // namespace
