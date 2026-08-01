/**
 * @file get_resource_handler_test.cpp
 * @brief Tests tenant-safe GetResource application orchestration.
 */

#include "haven/application/resources/get_resource_handler.hpp"

#include "haven/application/resources/resource_query_repository.hpp"
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

class InMemoryResourceRepository final : public ResourceQueryRepository {
public:
    void add(haven::domain::OrganizationId organization_id,
             haven::domain::ResourceId resource_id,
             haven::domain::Resource resource) {
        resources_.push_back(StoredResource{
            std::move(organization_id), std::move(resource_id), std::move(resource)});
    }

    [[nodiscard]] ResourceQueryResult find_by_id(
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

        return resource->resource;
    }

private:
    struct StoredResource final {
        haven::domain::OrganizationId organization_id;
        haven::domain::ResourceId resource_id;
        haven::domain::Resource resource;
    };

    std::vector<StoredResource> resources_;
};

class TenantLeakingResourceRepository final : public ResourceQueryRepository {
public:
    explicit TenantLeakingResourceRepository(haven::domain::Resource resource)
        : resource_(std::move(resource)) {}

    [[nodiscard]] ResourceQueryResult find_by_id(const haven::domain::OrganizationId&,
                                                 const haven::domain::ResourceId&) const override {
        return resource_;
    }

private:
    haven::domain::Resource resource_;
};

[[nodiscard]] haven::domain::Resource make_resource(
    const haven::domain::ResourceId& resource_id,
    const haven::domain::OrganizationId& organization_id) {
    return haven::domain::Resource{organization_id,
                                   resource_id,
                                   haven::domain::ResourceType::MeetingRoom,
                                   haven::domain::ResourceStatus::Active,
                                   false};
}

TEST(GetResourceHandlerTest, Handle_ShouldReturnResource_WhenResourceBelongsToOrganization) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = InMemoryResourceRepository{};
    repository.add(organization_id, resource_id, make_resource(resource_id, organization_id));
    const auto handler = GetResourceHandler{repository};

    const auto result = handler.handle(GetResourceQuery{organization_id, resource_id});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->organization_id(), organization_id);
}

TEST(GetResourceHandlerTest, Handle_ShouldReturnEmpty_WhenResourceDoesNotExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-missing"};
    auto repository = InMemoryResourceRepository{};
    const auto handler = GetResourceHandler{repository};

    const auto result = handler.handle(GetResourceQuery{organization_id, resource_id});

    EXPECT_FALSE(result.has_value());
}

TEST(GetResourceHandlerTest, Handle_ShouldReturnEmpty_WhenResourceBelongsToAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = InMemoryResourceRepository{};
    repository.add(
        owner_organization_id, resource_id, make_resource(resource_id, owner_organization_id));
    const auto handler = GetResourceHandler{repository};

    const auto result = handler.handle(GetResourceQuery{caller_organization_id, resource_id});

    EXPECT_FALSE(result.has_value());
}

TEST(GetResourceHandlerTest, Handle_ShouldReturnEmpty_WhenRepositoryReturnsCrossTenantResource) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository =
        TenantLeakingResourceRepository{make_resource(resource_id, owner_organization_id)};
    const auto handler = GetResourceHandler{repository};

    const auto result = handler.handle(GetResourceQuery{caller_organization_id, resource_id});

    EXPECT_FALSE(result.has_value());
}

}  // namespace
}  // namespace haven::application::resources
