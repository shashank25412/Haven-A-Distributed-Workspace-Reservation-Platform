/**
 * @file cached_resource_query_repository_test.cpp
 * @brief Tests cache-backed Resource query orchestration.
 */

#include "haven/application/resources/cached_resource_query_repository.hpp"

#include <gtest/gtest.h>

namespace haven::application::resources {
namespace {

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

const haven::domain::OrganizationId organization_id{"org"};
const haven::domain::ResourceId resource_id{"resource"};

TEST(CachedResourceQueryRepositoryTest, HitSkipsAuthoritativeLookup) {
    FakeCache cache;
    cache.result = resource();
    FakeQuery query;
    const CachedResourceQueryRepository repository{cache, query};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 0);
}

TEST(CachedResourceQueryRepositoryTest, MissLoadsAndStoresAuthoritativeResource) {
    FakeCache cache;
    FakeQuery query;
    query.result = resource();
    const CachedResourceQueryRepository repository{cache, query};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 1);
    EXPECT_EQ(cache.stores, 1);
}

TEST(CachedResourceQueryRepositoryTest, MissingResourceIsNotNegativeCached) {
    FakeCache cache;
    FakeQuery query;
    const CachedResourceQueryRepository repository{cache, query};
    EXPECT_FALSE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(cache.stores, 0);
}

TEST(CachedResourceQueryRepositoryTest, LookupFailureFallsBack) {
    FakeCache cache;
    cache.find_fails = true;
    FakeQuery query;
    query.result = resource();
    const CachedResourceQueryRepository repository{cache, query};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
    EXPECT_EQ(query.calls, 1);
}

TEST(CachedResourceQueryRepositoryTest, StoreFailureStillReturnsAuthoritativeResource) {
    FakeCache cache;
    cache.store_fails = true;
    FakeQuery query;
    query.result = resource();
    const CachedResourceQueryRepository repository{cache, query};
    EXPECT_TRUE(repository.find_by_id(organization_id, resource_id).has_value());
}

TEST(CachedResourceQueryRepositoryTest, AuthoritativeFailurePropagates) {
    FakeCache cache;
    FakeQuery query;
    query.fail = true;
    const CachedResourceQueryRepository repository{cache, query};
    EXPECT_THROW(repository.find_by_id(organization_id, resource_id), std::runtime_error);
}

}  // namespace
}  // namespace haven::application::resources
