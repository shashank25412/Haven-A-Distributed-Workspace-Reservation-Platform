/**
 * @file search_resources_handler_test.cpp
 * @brief Tests tenant-safe SearchResources application orchestration.
 */

#include "haven/application/resources/search_resources_handler.hpp"

#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace haven::application::resources {
namespace {

class InMemoryResourceRepository final : public ResourceRepository {
public:
    void add(haven::domain::OrganizationId organization_id,
             haven::domain::ResourceId resource_id,
             haven::domain::ResourceType resource_type,
             bool active,
             haven::domain::Resource resource) {
        resources_.push_back(StoredResource{std::move(organization_id),
                                            std::move(resource_id),
                                            resource_type,
                                            active,
                                            std::move(resource)});
    }

    [[nodiscard]] ResourceLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override {
        const auto resource =
            std::find_if(resources_.cbegin(),
                         resources_.cend(),
                         [&organization_id, &resource_id](const StoredResource& stored_resource) {
                             return stored_resource.organization_id == organization_id &&
                                    stored_resource.resource_id == resource_id;
                         });

        if (resource == resources_.cend()) {
            return std::nullopt;
        }

        return LoadedResource{resource->resource,
                              haven::application::persistence::PersistenceToken{1}};
    }

    [[nodiscard]] ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceType resource_type) const override {
        auto result = ResourceSearchResult{};

        for (const auto& stored_resource : resources_) {
            if (stored_resource.organization_id == organization_id &&
                stored_resource.resource_type == resource_type && stored_resource.active) {
                result.push_back(stored_resource.resource);
            }
        }

        return result;
    }

private:
    struct StoredResource final {
        haven::domain::OrganizationId organization_id;
        haven::domain::ResourceId resource_id;
        haven::domain::ResourceType resource_type;
        bool active;
        haven::domain::Resource resource;
    };

    std::vector<StoredResource> resources_;
};

class TenantLeakingResourceRepository final : public ResourceRepository {
public:
    explicit TenantLeakingResourceRepository(ResourceSearchResult resources)
        : resources_(std::move(resources)) {}

    [[nodiscard]] ResourceLookupResult find_by_id(const haven::domain::OrganizationId&,
                                                  const haven::domain::ResourceId&) const override {
        return std::nullopt;
    }

    [[nodiscard]] ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId&, haven::domain::ResourceType) const override {
        return resources_;
    }

private:
    ResourceSearchResult resources_;
};

[[nodiscard]] haven::domain::Resource make_resource(
    const haven::domain::ResourceId& resource_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceType resource_type,
    const bool requires_approval = false) {
    return haven::domain::Resource{organization_id,
                                   resource_id,
                                   resource_type,
                                   haven::domain::ResourceStatus::Active,
                                   requires_approval};
}

TEST(SearchResourcesHandlerTest, Handle_ShouldReturnMatchingResources_WhenActiveResourcesExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto first_resource_id = haven::domain::ResourceId{"resource-boardroom"};
    const auto second_resource_id = haven::domain::ResourceId{"resource-huddle-room"};
    auto repository = InMemoryResourceRepository{};
    repository.add(
        organization_id,
        first_resource_id,
        haven::domain::ResourceType::MeetingRoom,
        true,
        make_resource(
            first_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom));
    repository.add(
        organization_id,
        second_resource_id,
        haven::domain::ResourceType::MeetingRoom,
        true,
        make_resource(
            second_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom));
    const auto handler = SearchResourcesHandler{repository};

    const auto result = handler.handle(
        SearchResourcesQuery{organization_id, haven::domain::ResourceType::MeetingRoom});

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result.at(0).organization_id(), organization_id);
    EXPECT_EQ(result.at(1).organization_id(), organization_id);
}

TEST(SearchResourcesHandlerTest, Handle_ShouldReturnEmpty_WhenNoMatchingResourcesExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryResourceRepository{};
    const auto handler = SearchResourcesHandler{repository};

    const auto result = handler.handle(
        SearchResourcesQuery{organization_id, haven::domain::ResourceType::MeetingRoom});

    EXPECT_TRUE(result.empty());
}

TEST(SearchResourcesHandlerTest, Handle_ShouldExcludeInactiveResources_WhenMatchingResourcesExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = InMemoryResourceRepository{};
    repository.add(
        organization_id,
        resource_id,
        haven::domain::ResourceType::MeetingRoom,
        false,
        make_resource(resource_id, organization_id, haven::domain::ResourceType::MeetingRoom));
    const auto handler = SearchResourcesHandler{repository};

    const auto result = handler.handle(
        SearchResourcesQuery{organization_id, haven::domain::ResourceType::MeetingRoom});

    EXPECT_TRUE(result.empty());
}

TEST(SearchResourcesHandlerTest, Handle_ShouldExcludeDifferentResourceTypes_WhenResourcesExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-parking-slot"};
    auto repository = InMemoryResourceRepository{};
    repository.add(
        organization_id,
        resource_id,
        haven::domain::ResourceType::ParkingSlot,
        true,
        make_resource(resource_id, organization_id, haven::domain::ResourceType::ParkingSlot));
    const auto handler = SearchResourcesHandler{repository};

    const auto result = handler.handle(
        SearchResourcesQuery{organization_id, haven::domain::ResourceType::MeetingRoom});

    EXPECT_TRUE(result.empty());
}

TEST(SearchResourcesHandlerTest,
     Handle_ShouldReturnEmpty_WhenResourcesBelongToAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = InMemoryResourceRepository{};
    repository.add(
        owner_organization_id,
        resource_id,
        haven::domain::ResourceType::MeetingRoom,
        true,
        make_resource(
            resource_id, owner_organization_id, haven::domain::ResourceType::MeetingRoom));
    const auto handler = SearchResourcesHandler{repository};

    const auto result = handler.handle(
        SearchResourcesQuery{caller_organization_id, haven::domain::ResourceType::MeetingRoom});

    EXPECT_TRUE(result.empty());
}

TEST(SearchResourcesHandlerTest,
     Handle_ShouldRemoveCrossTenantResources_WhenRepositoryLeaksResources) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = TenantLeakingResourceRepository{ResourceSearchResult{make_resource(
        resource_id, owner_organization_id, haven::domain::ResourceType::MeetingRoom)}};
    const auto handler = SearchResourcesHandler{repository};

    const auto result = handler.handle(
        SearchResourcesQuery{caller_organization_id, haven::domain::ResourceType::MeetingRoom});

    EXPECT_TRUE(result.empty());
}

}  // namespace
}  // namespace haven::application::resources
