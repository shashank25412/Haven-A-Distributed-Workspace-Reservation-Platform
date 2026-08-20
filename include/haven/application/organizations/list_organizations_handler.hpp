/**
 * @file list_organizations_handler.hpp
 * @brief Declares the ListOrganizations application handler.
 */

#pragma once

#include "haven/application/organizations/organization_repository.hpp"
#include "haven/domain/organization.hpp"

#include <vector>

namespace haven::application::organizations {

/**
 * @brief Lists every organization in the directory.
 */
class ListOrganizationsHandler final {
public:
    explicit ListOrganizationsHandler(OrganizationRepository& organization_repository) noexcept;

    /**
     * @brief Executes the listing.
     *
     * @return Organizations ordered by display name.
     */
    [[nodiscard]] std::vector<haven::domain::Organization> handle() const;

private:
    OrganizationRepository& organization_repository_;
};

}  // namespace haven::application::organizations
