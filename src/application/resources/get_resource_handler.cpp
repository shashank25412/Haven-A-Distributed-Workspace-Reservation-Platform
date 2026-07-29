/**
 * @file get_resource_handler.cpp
 * @brief Implements the GetResource application use case.
 */

#include "haven/application/resources/get_resource_handler.hpp"

#include "haven/logging/logging.hpp"

#include <utility>

namespace haven::application::resources {

haven::application::resources::GetResourceHandler::GetResourceHandler(
    haven::application::resources::ResourceRepository& resource_repository) noexcept
    : resource_repository_(resource_repository) {}

std::optional<haven::domain::Resource> haven::application::resources::GetResourceHandler::handle(
    const haven::application::resources::GetResourceQuery& query) const {
    HVN_TRACE_SCOPE();

    auto resource = resource_repository_.find_by_id(query.organization_id(), query.resource_id());

    if (!resource.has_value()) {
        return std::nullopt;
    }

    if (resource->aggregate().organization_id() != query.organization_id()) {
        HVN_WARN_LOG("Resource repository returned a resource outside the requested tenant scope");
        return std::nullopt;
    }

    return std::move(resource->aggregate());
}

}  // namespace haven::application::resources
