/**
 * @file resource_test.cpp
 * @brief Tests the reservable resource domain aggregate.
 */

#include "haven/domain/resource.hpp"

#include <gtest/gtest.h>

namespace haven::domain {
namespace {

TEST(ResourceTest, Constructor_ShouldStoreIdentity_WhenResourceIsCreated) {
    const Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::MeetingRoom,
        ResourceStatus::Active,
        false};

    EXPECT_EQ(resource.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(resource.resource_id(), ResourceId{"resource-123"});
}

TEST(ResourceTest, Constructor_ShouldStoreType_WhenResourceIsCreated) {
    const Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::ParkingSlot,
        ResourceStatus::Active,
        false};

    EXPECT_EQ(resource.type(), ResourceType::ParkingSlot);
}

TEST(ResourceTest, IsActive_ShouldReturnTrue_WhenResourceStatusIsActive) {
    const Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::OfficeDesk,
        ResourceStatus::Active,
        false};

    EXPECT_TRUE(resource.is_active());
}

TEST(ResourceTest, IsActive_ShouldReturnFalse_WhenResourceStatusIsInactive) {
    const Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::OfficeDesk,
        ResourceStatus::Inactive,
        false};

    EXPECT_FALSE(resource.is_active());
}

TEST(ResourceTest, RequiresApproval_ShouldReturnTrue_WhenResourceRequiresApproval) {
    const Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::MeetingRoom,
        ResourceStatus::Active,
        true};

    EXPECT_TRUE(resource.requires_approval());
}

TEST(ResourceTest, Deactivate_ShouldMarkResourceInactive_WhenResourceIsActive) {
    Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::MeetingRoom,
        ResourceStatus::Active,
        false};

    resource.deactivate();

    EXPECT_EQ(resource.status(), ResourceStatus::Inactive);
    EXPECT_FALSE(resource.is_active());
}

TEST(ResourceTest, Activate_ShouldMarkResourceActive_WhenResourceIsInactive) {
    Resource resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::MeetingRoom,
        ResourceStatus::Inactive,
        false};

    resource.activate();

    EXPECT_EQ(resource.status(), ResourceStatus::Active);
    EXPECT_TRUE(resource.is_active());
}

TEST(ResourceTest, Rehydrate_ShouldRestorePersistedState) {
    const Resource resource = Resource::rehydrate(
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        "Atlas Meeting Room",
        "",
        ResourceType::MeetingRoom,
        ResourceStatus::Inactive,
        true,
        Version{42});

    EXPECT_EQ(resource.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(resource.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(resource.name(), "Atlas Meeting Room");
    EXPECT_TRUE(resource.description().empty());
    EXPECT_EQ(resource.type(), ResourceType::MeetingRoom);
    EXPECT_EQ(resource.status(), ResourceStatus::Inactive);
    EXPECT_TRUE(resource.requires_approval());
    EXPECT_EQ(resource.version(), Version{42});
}

TEST(ResourceTest, Rehydrate_ShouldRejectEmptyName) {
    EXPECT_THROW(
        static_cast<void>(Resource::rehydrate(
            OrganizationId{"organization-123"},
            ResourceId{"resource-123"},
            "",
            "",
            ResourceType::MeetingRoom,
            ResourceStatus::Active,
            false,
            Version{42})),
        std::invalid_argument);
}

TEST(ResourceTest, Rehydrate_ShouldRejectZeroPersistenceVersion) {
    EXPECT_THROW(
        static_cast<void>(Resource::rehydrate(
            OrganizationId{"organization-123"},
            ResourceId{"resource-123"},
            "Atlas Meeting Room",
            "",
            ResourceType::MeetingRoom,
            ResourceStatus::Active,
            false,
            Version{0})),
        std::invalid_argument);
}

}  // namespace
}  // namespace haven::domain
