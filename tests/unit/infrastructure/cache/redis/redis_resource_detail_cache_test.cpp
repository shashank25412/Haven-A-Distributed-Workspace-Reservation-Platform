/**
 * @file redis_resource_detail_cache_test.cpp
 * @brief Tests Redis Resource detail cache behavior.
 */

#include "haven/infrastructure/cache/redis/cached_resource_record.hpp"
#include "haven/infrastructure/cache/redis/resource_detail_cache_key.hpp"

#include <gtest/gtest.h>

namespace haven::infrastructure::cache::redis {
namespace {

haven::domain::Resource resource() {
    return haven::domain::Resource::rehydrate(haven::domain::OrganizationId{"org:a"},
                                              haven::domain::ResourceId{"res*1"},
                                              "Room",
                                              "Description",
                                              haven::domain::ResourceType::MeetingRoom,
                                              haven::domain::ResourceStatus::Active,
                                              true,
                                              haven::domain::Version{7});
}

TEST(ResourceDetailCacheKeyTest, IncludesTenantAndEscapesDelimitersAndGlobCharacters) {
    EXPECT_EQ(resource_detail_cache_key(haven::domain::OrganizationId{"org:a"},
                                        haven::domain::ResourceId{"res*1"}),
              "haven:v1:org:org%3Aa:resource:res%2A1");
    EXPECT_NE(resource_detail_cache_key(haven::domain::OrganizationId{"org-a"},
                                        haven::domain::ResourceId{"same"}),
              resource_detail_cache_key(haven::domain::OrganizationId{"org-b"},
                                        haven::domain::ResourceId{"same"}));
}

TEST(CachedResourceRecordTest, RoundTripPreservesRequiredAggregateStateWithoutCas) {
    const auto payload = serialize_cached_resource(resource());
    EXPECT_EQ(payload.find("cas"), std::string::npos);
    EXPECT_EQ(payload.find("persistenceToken"), std::string::npos);
    EXPECT_EQ(deserialize_cached_resource(payload), resource());
}

TEST(CachedResourceRecordTest, RejectsCorruptAndUnsupportedPayloads) {
    EXPECT_THROW(deserialize_cached_resource("not-json"), std::exception);
    EXPECT_THROW(deserialize_cached_resource("{\"schemaVersion\":2}"), std::exception);
}

}  // namespace
}  // namespace haven::infrastructure::cache::redis
