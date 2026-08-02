/**
 * @file authoritative_resource_query_repository.cpp
 * @brief Implements authoritative Resource queries.
 */

#include "haven/application/resources/authoritative_resource_query_repository.hpp"

#include <utility>

namespace haven::application::resources {

AuthoritativeResourceQueryRepository::AuthoritativeResourceQueryRepository(
    ResourceRepository& repository) noexcept
    : repository_(repository) {}

ResourceQueryResult AuthoritativeResourceQueryRepository::find_by_id(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) const {
    auto loaded = repository_.find_by_id(organization_id, resource_id);
    if (!loaded.has_value()) {
        return std::nullopt;
    }
    return std::move(loaded->aggregate());
}

}  // namespace haven::application::resources
