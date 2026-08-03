/**
 * @file redis_resource_detail_cache_test.cpp
 * @brief Tests Redis Resource detail caching against a live service.
 */

#include "haven/infrastructure/cache/redis/redis_resource_detail_cache.hpp"

#include "haven/application/resources/cached_resource_query_repository.hpp"
#include "haven/infrastructure/cache/redis/redis_connection.hpp"
#include "haven/infrastructure/cache/redis/resource_detail_cache_key.hpp"
#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"
#include "haven/infrastructure/observability/metrics/prometheus_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <sw/redis++/redis++.h>
#include <thread>

namespace haven::infrastructure::cache::redis {
namespace {
using namespace std::chrono_literals;

RedisConfiguration configuration(std::chrono::seconds ttl = 300s) {
    return {true, "tcp://127.0.0.1:6379", "", 100ms, 100ms, ttl};
}

haven::domain::Resource resource(const char* organization = "redis-test-org") {
    return haven::domain::Resource::rehydrate(haven::domain::OrganizationId{organization},
                                              haven::domain::ResourceId{"resource-1"},
                                              "Room",
                                              "",
                                              haven::domain::ResourceType::MeetingRoom,
                                              haven::domain::ResourceStatus::Active,
                                              false,
                                              haven::domain::Version{1});
}

class CountingQuery final : public haven::application::resources::ResourceQueryRepository {
public:
    mutable int calls{};
    haven::application::resources::ResourceQueryResult find_by_id(
        const haven::domain::OrganizationId&, const haven::domain::ResourceId&) const override {
        ++calls;
        return resource();
    }
};

class RedisResourceDetailCacheIntegrationTest : public ::testing::Test {
protected:
    RedisResourceDetailCacheIntegrationTest()
        : connection(std::make_shared<RedisConnection>(configuration())),
          cache(connection, configuration(), metrics) {}
    void SetUp() override {
        connection->client().flushdb();
    }
    std::shared_ptr<RedisConnection> connection;
    haven::infrastructure::observability::metrics::PrometheusMetricsRecorder metrics;
    RedisResourceDetailCache cache;
};

TEST_F(RedisResourceDetailCacheIntegrationTest, StoreFindMissingTenantSeparationAndErase) {
    const haven::domain::OrganizationId org{"redis-test-org"};
    const haven::domain::ResourceId id{"resource-1"};
    EXPECT_FALSE(cache.find(org, id).has_value());
    cache.store(org, resource());
    EXPECT_TRUE(cache.find(org, id).has_value());
    EXPECT_FALSE(cache.find(haven::domain::OrganizationId{"other-org"}, id).has_value());
    cache.erase(org, id);
    EXPECT_FALSE(cache.find(org, id).has_value());
    const auto output = metrics.collect();
    EXPECT_NE(output.find("operation=\"find\",outcome=\"hit\""), std::string::npos);
    EXPECT_NE(output.find("operation=\"find\",outcome=\"miss\""), std::string::npos);
    EXPECT_NE(output.find("operation=\"store\",outcome=\"success\""), std::string::npos);
    EXPECT_NE(output.find("operation=\"erase\",outcome=\"success\""), std::string::npos);
}

TEST(RedisResourceDetailCacheTtlIntegrationTest, EntryExpires) {
    auto connection = std::make_shared<RedisConnection>(configuration(1s));
    auto metrics = haven::infrastructure::observability::metrics::NoOpMetricsRecorder{};
    RedisResourceDetailCache cache{connection, configuration(1s), metrics};
    const haven::domain::OrganizationId org{"redis-test-org"};
    const haven::domain::ResourceId id{"resource-1"};
    connection->client().flushdb();
    cache.store(org, resource());
    std::this_thread::sleep_for(1200ms);
    EXPECT_FALSE(cache.find(org, id).has_value());
}

TEST_F(RedisResourceDetailCacheIntegrationTest, CorruptAndUnsupportedEntriesAreDeleted) {
    const haven::domain::OrganizationId org{"redis-test-org"};
    const haven::domain::ResourceId id{"resource-1"};
    const auto key = resource_detail_cache_key(org, id);
    connection->client().set(key, "broken");
    EXPECT_FALSE(cache.find(org, id).has_value());
    EXPECT_FALSE(connection->client().exists(key));
    connection->client().set(key, "{\"schemaVersion\":2}");
    EXPECT_FALSE(cache.find(org, id).has_value());
    EXPECT_FALSE(connection->client().exists(key));
}

TEST_F(RedisResourceDetailCacheIntegrationTest, CachedQueryReadsAuthoritativeOnlyOnce) {
    CountingQuery authoritative;
    auto query_metrics = haven::infrastructure::observability::metrics::NoOpMetricsRecorder{};
    haven::application::resources::CachedResourceQueryRepository query{
        cache, authoritative, query_metrics};
    const haven::domain::OrganizationId org{"redis-test-org"};
    const haven::domain::ResourceId id{"resource-1"};
    EXPECT_TRUE(query.find_by_id(org, id).has_value());
    EXPECT_TRUE(query.find_by_id(org, id).has_value());
    EXPECT_EQ(authoritative.calls, 1);
}

}  // namespace
}  // namespace haven::infrastructure::cache::redis
