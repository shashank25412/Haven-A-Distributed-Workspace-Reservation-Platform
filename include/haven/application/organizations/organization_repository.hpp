/**
 * @file organization_repository.hpp
 * @brief Defines directory-wide organization retrieval required by the application layer.
 */

#pragma once

#include "haven/domain/organization.hpp"

#include <vector>

namespace haven::application::organizations {

/**
 * @brief Provides organization directory persistence operations.
 *
 * Unlike resource and reservation repositories, this port is intentionally
 * not tenant-scoped: listing organizations is how a caller discovers tenant
 * identifiers and their human-readable names in the first place.
 */
class OrganizationRepository {
public:
    virtual ~OrganizationRepository() = default;

    /**
     * @brief Returns every organization in the directory.
     *
     * @return Organizations ordered by display name.
     */
    [[nodiscard]] virtual std::vector<haven::domain::Organization> find_all() const = 0;
};

}  // namespace haven::application::organizations
