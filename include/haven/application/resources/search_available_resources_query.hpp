/**
 * @file search_available_resources_query.hpp
 * @brief Defines the query for searching available resources.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

#include <utility>

namespace haven::application::resources {

/**
 * @brief Describes a tenant-scoped resource availability search.
 */
class SearchAvailableResourcesQuery final {
public:
    /**
     * @brief Constructs an available-resource search query.
     *
     * @param organization_id Organization used to scope the search.
     * @param resource_type Requested resource type.
     * @param interval Requested reservation interval.
     */
    SearchAvailableResourcesQuery(
        haven::domain::OrganizationId organization_id,
        haven::domain::ResourceType resource_type,
        haven::domain::TimeInterval interval)
        : organization_id_(std::move(organization_id)),
          resource_type_(resource_type),
          interval_(std::move(interval)) {}

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

    /**
     * @brief Returns the requested availability interval.
     */
    [[nodiscard]] const haven::domain::TimeInterval& interval() const noexcept {
        return interval_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ResourceType resource_type_;
    haven::domain::TimeInterval interval_;
};

}  // namespace haven::application::resources
