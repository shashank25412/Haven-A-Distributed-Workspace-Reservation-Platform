/**
 * @file search_resources_handler.hpp
 * @brief Declares the SearchResources application use-case handler.
 */

#pragma once

#include "haven/application/resources/resource_repository.hpp"
#include "haven/application/resources/search_resources_query.hpp"

namespace haven::application::resources {

/**
 * @brief Searches active resources while preserving tenant isolation.
 *
 * The handler depends only on application-owned ports and domain types.
 */
class SearchResourcesHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param resource_repository Tenant-aware resource repository.
     */
    explicit SearchResourcesHandler(ResourceRepository& resource_repository) noexcept;

    /**
     * @brief Executes the tenant-scoped resource search.
     *
     * @param query Organization and resource type search criteria.
     * @return Active matching resources visible within the organization.
     */
    [[nodiscard]] ResourceSearchResult handle(const SearchResourcesQuery& query) const;

private:
    ResourceRepository& resource_repository_;
};

}  // namespace haven::application::resources