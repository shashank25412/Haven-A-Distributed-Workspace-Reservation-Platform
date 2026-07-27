/**
 * @file get_resource_query.hpp
 * @brief Defines the input query for retrieving one tenant-scoped resource.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"

#include <utility>

namespace haven::application::resources {

/**
 * @brief Describes a request to retrieve one resource within an organization.
 */
class GetResourceQuery final {
public:
    /**
     * @brief Constructs a tenant-scoped resource query.
     *
     * @param organization_id Organization visible to the caller.
     * @param resource_id Resource identifier requested by the caller.
     */
    GetResourceQuery(
        haven::domain::OrganizationId organization_id,
        haven::domain::ResourceId resource_id)
        : organization_id_(std::move(organization_id)),
          resource_id_(std::move(resource_id)) {}

    /**
     * @brief Returns the organization used to scope the lookup.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the requested resource identifier.
     */
    [[nodiscard]] const haven::domain::ResourceId& resource_id() const noexcept {
        return resource_id_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ResourceId resource_id_;
};

}  // namespace haven::application::resources