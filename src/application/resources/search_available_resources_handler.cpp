/**
 * @file search_available_resources_handler.cpp
 * @brief Implements the SearchAvailableResources application use case.
 */

#include "haven/application/resources/search_available_resources_handler.hpp"

#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::resources {

SearchAvailableResourcesHandler::SearchAvailableResourcesHandler(
    ResourceRepository& resource_repository,
    reservations::ReservationRepository& reservation_repository) noexcept
    : resource_repository_(resource_repository),
      reservation_repository_(reservation_repository) {}

ResourceSearchResult SearchAvailableResourcesHandler::handle(
    const SearchAvailableResourcesQuery& query) const {
    HVN_TRACE_SCOPE();

    auto resources = resource_repository_.find_active_by_type(
        query.organization_id(),
        query.resource_type());

    const auto first_unavailable_resource = std::remove_if(
        resources.begin(),
        resources.end(),
        [this, &query](const haven::domain::Resource& resource) {
            const auto outside_requested_scope =
                resource.organization_id() != query.organization_id()
                || resource.type() != query.resource_type()
                || resource.status() != haven::domain::ResourceStatus::Active;

            if (outside_requested_scope) {
                HVN_WARN_LOG(
                    "Resource repository returned an entry outside the requested search scope");
                return true;
            }

            return reservation_repository_.has_conflict(
                query.organization_id(),
                resource.resource_id(),
                query.interval());
        });

    resources.erase(first_unavailable_resource, resources.end());

    return resources;
}

}  // namespace haven::application::resources
