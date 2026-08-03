/** @file couchbase_outbox_repository_test.cpp @brief Live publisher Outbox repository tests. */
#include "haven/infrastructure/persistence/couchbase/couchbase_outbox_repository.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/events/reservation_created_event.hpp"
#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_cas.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace cb = haven::infrastructure::persistence::couchbase;
[[nodiscard]] std::optional<std::string> env(const char* n) {
    const auto* v = std::getenv(n);
    return v && *v ? std::optional<std::string>{v} : std::nullopt;
}
[[nodiscard]] std::optional<cb::CouchbaseConfiguration> config() {
    auto c = env("HVN_COUCHBASE_CONNECTION_STRING"), u = env("HVN_COUCHBASE_USERNAME"),
         p = env("HVN_COUCHBASE_PASSWORD"), b = env("HVN_COUCHBASE_BUCKET"),
         s = env("HVN_COUCHBASE_SCOPE");
    if (!c || !u || !p || !b || !s)
        return std::nullopt;
    return cb::CouchbaseConfiguration{*c, *u, *p, *b, *s};
}
[[nodiscard]] std::string unique() {
    static unsigned n{};
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
           std::to_string(++n);
}

class CouchbaseOutboxRepositoryIntegrationTest : public testing::Test {
protected:
    void SetUp() override {
        auto c = config();
        if (!c)
            GTEST_SKIP() << "Set HVN_COUCHBASE_*";
        connection = std::make_shared<cb::CouchbaseConnection>(*c);
        repository = std::make_unique<cb::CouchbaseOutboxRepository>(connection, metrics);
    }
    void TearDown() override {
        if (!connection)
            return;
        auto collection = connection->collection(cb::CouchbaseCollections::outbox);
        for (const auto& k : keys) {
            auto [e, r] = collection.remove(k).get();
            static_cast<void>(r);
            if (e && e.ec() != ::couchbase::errc::key_value::document_not_found)
                ADD_FAILURE() << e.ec().message();
        }
    }
    [[nodiscard]] cb::OutboxDocument make(const haven::domain::OrganizationId& org,
                                          const haven::domain::EventId& eid,
                                          std::chrono::system_clock::time_point when) {
        auto rid = haven::domain::ReservationId{"reservation-" + eid.value()};
        auto event = haven::domain::ReservationCreatedEvent{
            eid,
            when,
            org,
            rid,
            haven::domain::ResourceId{"resource"},
            haven::domain::UserId{"user"},
            haven::domain::TimeInterval{when, when + std::chrono::hours{1}},
            haven::domain::ReservationKind::Standard,
            haven::domain::ReservationStatus::Confirmed};
        return cb::to_outbox_document(org, rid, event);
    }
    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        cb::OutboxDocument document) {
        auto key = cb::outbox_document_key(document.organization_id, document.event_id);
        keys.push_back(key);
        auto [e, r] = connection->collection(cb::CouchbaseCollections::outbox)
                          .insert(key, cb::outbox_document_to_json(document))
                          .get();
        if (e) {
            throw std::runtime_error{"Fixture insert failed: " + e.ec().message()};
        }
        return cb::persistence_token_from(r.cas());
    }
    [[nodiscard]] cb::OutboxDocument read(const haven::domain::OrganizationId& o,
                                          const haven::domain::EventId& e) {
        auto [er, r] = connection->collection(cb::CouchbaseCollections::outbox)
                           .get(cb::outbox_document_key(o, e))
                           .get();
        EXPECT_FALSE(er);
        return cb::outbox_document_from_json(r.content_as<tao::json::value>());
    }
    std::shared_ptr<cb::CouchbaseConnection> connection;
    haven::infrastructure::observability::metrics::NoOpMetricsRecorder metrics;
    std::unique_ptr<cb::CouchbaseOutboxRepository> repository;
    std::vector<std::string> keys;
};

TEST_F(CouchbaseOutboxRepositoryIntegrationTest,
       PendingQueryFiltersLimitsAndOrdersDeterministically) {
    auto u = unique();
    auto org = haven::domain::OrganizationId{"org-" + u};
    auto t = std::chrono::system_clock::time_point{std::chrono::seconds{1'800'000'000}};
    auto b = make(org, haven::domain::EventId{"b-" + u}, t);
    static_cast<void>(insert(b));
    auto a = make(org, haven::domain::EventId{"a-" + u}, t);
    static_cast<void>(insert(a));
    auto later = make(org, haven::domain::EventId{"c-" + u}, t + std::chrono::seconds{1});
    static_cast<void>(insert(later));
    auto publishing = make(org, haven::domain::EventId{"publishing-" + u}, t);
    publishing.status = cb::OutboxStatus::Publishing;
    static_cast<void>(insert(publishing));
    auto published = make(org, haven::domain::EventId{"published-" + u}, t);
    published.status = cb::OutboxStatus::Published;
    published.published_at = t;
    static_cast<void>(insert(published));
    auto found = repository->find_pending(2);
    ASSERT_EQ(found.size(), 2U);
    EXPECT_EQ(found[0].aggregate().event_id, a.event_id);
    EXPECT_EQ(found[1].aggregate().event_id, b.event_id);
    EXPECT_THROW(static_cast<void>(repository->find_pending(0)), std::invalid_argument);
}
TEST_F(CouchbaseOutboxRepositoryIntegrationTest, ClaimUsesCasIncrementsOnceAndIsTenantIsolated) {
    auto u = unique();
    auto org = haven::domain::OrganizationId{"org-" + u};
    auto other = haven::domain::OrganizationId{"other-" + u};
    auto eid = haven::domain::EventId{"event-" + u};
    auto t = std::chrono::system_clock::now();
    auto token = insert(make(org, eid, t));
    auto other_token = insert(make(other, eid, t));
    auto won = repository->claim(org, eid, token);
    ASSERT_TRUE(won);
    EXPECT_EQ(won->aggregate().attempt_count, 1U);
    EXPECT_NE(won->persistence_token(), token);
    EXPECT_FALSE(repository->claim(org, eid, token));
    EXPECT_EQ(read(org, eid).attempt_count, 1U);
    EXPECT_TRUE(repository->claim(other, eid, other_token));
}
TEST_F(CouchbaseOutboxRepositoryIntegrationTest,
       PublishAndRetryPreserveEnvelopeAndRejectInvalidOrStaleTransitions) {
    auto u = unique();
    auto org = haven::domain::OrganizationId{"org-" + u};
    auto t = std::chrono::system_clock::now();
    auto first = haven::domain::EventId{"first-" + u};
    auto pending_token = insert(make(org, first, t));
    EXPECT_THROW(static_cast<void>(repository->mark_published(org, first, pending_token, t)),
                 haven::application::RepositoryError);
    EXPECT_THROW(static_cast<void>(repository->release_for_retry(org, first, pending_token)),
                 haven::application::RepositoryError);
    auto claimed = repository->claim(org, first, pending_token);
    ASSERT_TRUE(claimed);
    auto published = repository->mark_published(
        org, first, claimed->persistence_token(), t + std::chrono::seconds{2});
    EXPECT_EQ(published.aggregate().serialized_envelope, claimed->aggregate().serialized_envelope);
    EXPECT_EQ(published.aggregate().attempt_count, 1U);
    EXPECT_NE(published.persistence_token(), claimed->persistence_token());
    auto published_document = read(org, first);
    EXPECT_EQ(published_document.status, cb::OutboxStatus::Published);
    EXPECT_EQ(published_document.published_at, t + std::chrono::seconds{2});
    auto pending_after_publish = repository->find_pending(100);
    EXPECT_TRUE(std::none_of(
        pending_after_publish.begin(), pending_after_publish.end(), [&](const auto& message) {
            return message.aggregate().organization_id == org &&
                   message.aggregate().event_id == first;
        }));
    EXPECT_FALSE(repository->claim(org, first, published.persistence_token()));
    EXPECT_THROW(static_cast<void>(repository->mark_published(
                     org, first, claimed->persistence_token(), t + std::chrono::seconds{3})),
                 haven::application::RepositoryError);
    EXPECT_THROW(
        static_cast<void>(repository->release_for_retry(org, first, published.persistence_token())),
        haven::application::RepositoryError);
    auto second = haven::domain::EventId{"second-" + u};
    auto second_token = insert(make(org, second, t));
    auto second_claim = repository->claim(org, second, second_token);
    ASSERT_TRUE(second_claim);
    EXPECT_THROW(static_cast<void>(repository->release_for_retry(org, second, second_token)),
                 haven::application::RepositoryError);
    auto released = repository->release_for_retry(org, second, second_claim->persistence_token());
    EXPECT_EQ(released.aggregate().attempt_count, 1U);
    EXPECT_EQ(released.aggregate().serialized_envelope,
              second_claim->aggregate().serialized_envelope);
    EXPECT_NE(released.persistence_token(), second_claim->persistence_token());
    EXPECT_EQ(read(org, second).status, cb::OutboxStatus::Pending);
}
TEST_F(CouchbaseOutboxRepositoryIntegrationTest, MalformedDocumentIsPersistenceFailure) {
    auto u = unique();
    auto org = haven::domain::OrganizationId{"org-" + u};
    auto eid = haven::domain::EventId{"bad-" + u};
    auto key = cb::outbox_document_key(org, eid);
    keys.push_back(key);
    auto [e, r] = connection->collection(cb::CouchbaseCollections::outbox)
                      .insert(key, tao::json::value{{"documentType", "outbox"}})
                      .get();
    ASSERT_FALSE(e);
    try {
        static_cast<void>(repository->claim(org, eid, cb::persistence_token_from(r.cas())));
        FAIL() << "Expected persistence failure";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::Persistence);
    }
}
}  // namespace
