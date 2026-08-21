/**
 * @file resource_repository.cpp
 * @brief Provides the default (no-op throw) body for ResourceRepository::save.
 */

#include "haven/application/resources/resource_repository.hpp"
#include "haven/application/repository_error.hpp"

namespace haven::application::resources {

void ResourceRepository::save(const haven::domain::Resource&) {
    throw RepositoryError{RepositoryErrorCode::Persistence, "save not implemented"};
}

ResourceSearchResult ResourceRepository::list_by_organization(
    const haven::domain::OrganizationId&) const {
    throw RepositoryError{RepositoryErrorCode::Persistence, "list_by_organization not implemented"};
}

void ResourceRepository::update_resource(const haven::domain::Resource&) {
    throw RepositoryError{RepositoryErrorCode::Persistence, "update_resource not implemented"};
}

void ResourceRepository::remove_resource(const haven::domain::OrganizationId&,
                                         const haven::domain::ResourceId&) {
    throw RepositoryError{RepositoryErrorCode::Persistence, "remove_resource not implemented"};
}

}  // namespace haven::application::resources
