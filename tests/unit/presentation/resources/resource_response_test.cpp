/**
 * @file resource_response_test.cpp
 * @brief Tests Resource detail response serialization.
 */

#include "haven/presentation/resources/resource_response.hpp"

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/version.hpp"

#include <gtest/gtest.h>

namespace haven::presentation::resources {
namespace {

TEST(ResourceResponseTest, ToJson_ShouldSerializeEveryPublicResourceField) {
    const auto resource = haven::domain::Resource::rehydrate(
        haven::domain::OrganizationId{"organization-1"},
        haven::domain::ResourceId{"resource-1"},
        "Orion",
        "Meeting room near reception.",
        haven::domain::ResourceType::MeetingRoom,
        haven::domain::ResourceStatus::Active,
        true,
        haven::domain::Version{7});

    const Json::Value response = ResourceResponse{resource}.to_json();

    EXPECT_EQ(response["organizationId"].asString(), "organization-1");
    EXPECT_EQ(response["resourceId"].asString(), "resource-1");
    EXPECT_EQ(response["name"].asString(), "Orion");
    EXPECT_EQ(response["description"].asString(), "Meeting room near reception.");
    EXPECT_EQ(response["resourceType"].asString(), "MEETING_ROOM");
    EXPECT_EQ(response["status"].asString(), "ACTIVE");
    EXPECT_TRUE(response["requiresApproval"].asBool());
    EXPECT_EQ(response["version"].asUInt64(), 7U);
    EXPECT_EQ(response.size(), 8U);
    EXPECT_FALSE(response.isMember("persistenceToken"));
    EXPECT_FALSE(response.isMember("cas"));
}

}  // namespace
}  // namespace haven::presentation::resources
