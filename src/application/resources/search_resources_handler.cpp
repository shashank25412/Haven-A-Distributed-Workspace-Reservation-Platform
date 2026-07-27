/**
 * @file search_resources_handler.cpp
 * @brief Implements the SearchResources application use case.
 */

#include "haven/application/resources/search_resources_handler.hpp"

#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::resources {

SearchResourcesHandler::SearchResourcesHandler(ResourceRepository& resource_repository) noexcept
    : resource_repository_(resource_repository) {}

ResourceSearchResult SearchResourcesHandler::handle(const SearchResourcesQuery& query) const {
    HVN_TRACE_SCOPE();

    auto resources = resource_repository_.find_active_by_type(
        query.organization_id(),
        query.resource_type());

    const auto first_hidden_resource = std::remove_if(
        resources.begin(),
        resources.end(),
        [&query](const haven::domain::Resource& resource) {
            return resource.organization_id() != query.organization_id();
        });

    if (first_hidden_resource != resources.end()) {
        HVN_WARN_LOG("Resource repository returned resources outside the requested tenant scope");
        resources.erase(first_hidden_resource, resources.end());
    }

    return resources;
}

}  // namespace haven::application::resources