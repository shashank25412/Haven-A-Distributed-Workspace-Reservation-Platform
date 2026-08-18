/**
 * @file cached_resource_query_repository_test.cpp
 * @brief Tests cache-backed Resource query orchestration.
 */

#include "haven/application/resources/cached_resource_query_repository.hpp"

#include "application/util/recording_metrics_recorder.hpp"

#include <gtest/gtest.h>

namespace haven::application::resources {
namespace {
using Metrics = haven::test::application::observability::metrics::RecordingMetricsRecorder;

haven::domain::Resource resource() {
    return haven::domain::Resource::rehydrate(haven::domain::OrganizationId{"org"},
                                              haven::domain::ResourceId{"resource"},
                                              "Room",
                                              "",
                                              haven::domain::ResourceType::MeetingRoom,
                                              haven::domain::ResourceStatus::Active,
                                              false,
                                              haven::domain::Version{2});
}

class FakeQuery final : public ResourceQueryRepository {
public:
    mutable int calls{};
    ResourceQueryResult result;
    bool fail{};
    ResourceQueryResult find_by_id(const haven::domain::OrganizationId&,
                                   const haven::domain::ResourceId&) const override {
        ++calls;
        if (fail)
            throw std::runtime_error{"authoritative failure"};
        return result;
    }
};

class FakeCache final : public ResourceDetailCache {
public:
    mutable int finds{};
    int stores{};
    std::optional<haven::domain::Resource> result;
    bool find_fails{};
    bool store_fails{};
    std::optional<haven::domain::Resource> find(const haven::domain::OrganizationId&,
                                                const haven::domain::ResourceId&) const override {
        ++finds;
        if (find_fails)
            throw ResourceDetailCacheError{"lookup"};
        return result;
    }
    void store(const haven::domain::OrganizationId&, const haven::domain::Resource&) override {
        ++stores;
        if (store_fails)
            throw ResourceDetailCacheError{"store"};
    }
    void erase(const haven::domain::OrganizationId&, const haven::domain::ResourceId&) override {}
};

class ThrowingMetrics final : public observability::metrics::MetricsRecorder {
public:
    void increment_counter(const observability::metrics::MetricName&,
                           double,
                           const observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics"};
    }
    void set_gauge(const observability::metrics::MetricName&,
                   double,
                   const observability::metrics::MetricLabels&) override {}
    void observe_duration(const observability::metrics::MetricName&,
                          std::chrono::microseconds,
                          const observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics"};
    }
};

const haven::domain::OrganizationId organization_id{"org"};
const haven::domain::ResourceId resource_id{"resource"};

TEST(CachedResourceQueryRepositoryTest, HitSkipsAuthoritativeLookup) {
    FakeCache cache;
    cache.result = resource();
    FakeQuery query;
    auto metrics = Metrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 0);
    ASSERT_EQ(metrics.counter_increments().size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].labels[0].value(), "cache_hit");
    ASSERT_EQ(metrics.duration_observations().size(), 1U);
}

TEST(CachedResourceQueryRepositoryTest, MissLoadsAndStoresAuthoritativeResource) {
    FakeCache cache;
    FakeQuery query;
    query.result = resource();
    auto metrics = Metrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 1);
    EXPECT_EQ(cache.stores, 1);
    ASSERT_EQ(metrics.counter_increments().size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].labels[0].value(), "authoritative_after_miss");
}

TEST(CachedResourceQueryRepositoryTest, MissingResourceIsNotNegativeCached) {
    FakeCache cache;
    FakeQuery query;
    auto metrics = Metrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_FALSE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(cache.stores, 0);
}

TEST(CachedResourceQueryRepositoryTest, LookupFailureFallsBack) {
    FakeCache cache;
    cache.find_fails = true;
    FakeQuery query;
    query.result = resource();
    auto metrics = Metrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 1);
    ASSERT_EQ(metrics.counter_increments().size(), 1U);
    EXPECT_EQ(metrics.counter_increments()[0].labels[0].value(),
              "authoritative_after_cache_failure");
}

TEST(CachedResourceQueryRepositoryTest, StoreFailureStillReturnsAuthoritativeResource) {
    FakeCache cache;
    cache.store_fails = true;
    FakeQuery query;
    query.result = resource();
    auto metrics = Metrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
}

TEST(CachedResourceQueryRepositoryTest, AuthoritativeFailurePropagates) {
    FakeCache cache;
    FakeQuery query;
    query.fail = true;
    auto metrics = Metrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_THROW(static_cast<void>(repository.find_by_id(organization_id, resource_id)),
                 std::runtime_error);
}

TEST(CachedResourceQueryRepositoryTest, MetricsFailuresDoNotAlterCacheHit) {
    FakeCache cache;
    cache.result = resource();
    FakeQuery query;
    auto metrics = ThrowingMetrics{};
    const CachedResourceQueryRepository repository{cache, query, metrics};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 0);
}

}  // namespace
}  // namespace haven::application::resources
