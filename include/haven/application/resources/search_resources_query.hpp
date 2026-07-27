/**
 * @file search_resources_query.hpp
 * @brief Defines the input query for searching tenant-scoped resources.
 */

#pragma once

#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/organization_id.hpp"

#include <utility>

namespace haven::application::resources {

/**
 * @brief Describes a tenant-scoped search for active resources of one type.
 */
class SearchResourcesQuery final {
public:
    /**
     * @brief Constructs a resource search query.
     *
     * @param organization_id Organization visible to the caller.
     * @param resource_type Resource type requested by the caller.
     */
    SearchResourcesQuery(
        haven::domain::OrganizationId organization_id,
        haven::domain::ResourceType resource_type)
        : organization_id_(std::move(organization_id)),
          resource_type_(resource_type) {}

    /**
     * @brief Returns the organization used to scope the search.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the requested resource type.
     */
    [[nodiscard]] haven::domain::ResourceType resource_type() const noexcept {
        return resource_type_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ResourceType resource_type_;
};

}  // namespace haven::application::resources