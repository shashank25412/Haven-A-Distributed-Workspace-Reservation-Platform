/**
 * @file resource_repository.hpp
 * @brief Defines tenant-scoped resource retrieval required by application use cases.
 */

#pragma once

#include "haven/application/persistence/loaded.hpp"
#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"

#include <optional>
#include <vector>

namespace haven::application::resources {

/**
 * @brief Represents the result of a tenant-scoped resource lookup.
 *
 * An empty result means that the resource either does not exist or is not
 * visible within the supplied organization.
 */
using LoadedResource = haven::application::persistence::Loaded<haven::domain::Resource>;
using ResourceLookupResult = std::optional<LoadedResource>;

/**
 * @brief Represents resources returned by a tenant-scoped search.
 */
using ResourceSearchResult = std::vector<haven::domain::Resource>;

/**
 * @brief Provides resource persistence operations required by the application layer.
 *
 * Every operation requires explicit organization context. Implementations must
 * never return resources belonging to another organization.
 */
class ResourceRepository {
public:
    /**
     * @brief Destroys the resource repository.
     */
    virtual ~ResourceRepository() = default;

    /**
     * @brief Finds a resource within an organization.
     *
     * @param organization_id Organization that owns the requested resource.
     * @param resource_id Resource identifier to retrieve.
     * @return The resource when visible in the organization; otherwise an empty result.
     */
    [[nodiscard]] virtual ResourceLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const = 0;

    /**
     * @brief Finds active resources of a given type within an organization.
     *
     * @param organization_id Organization used to scope the search.
     * @param resource_type Resource type requested by the caller.
     * @return Active matching resources belonging to the organization.
     */
    [[nodiscard]] virtual ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId& organization_id,
        haven::domain::ResourceType resource_type) const = 0;
};

}  // namespace haven::application::resources
