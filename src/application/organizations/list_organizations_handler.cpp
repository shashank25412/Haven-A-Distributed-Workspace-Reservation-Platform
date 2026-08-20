/**
 * @file list_organizations_handler.cpp
 * @brief Implements the ListOrganizations application handler.
 */

#include "haven/application/organizations/list_organizations_handler.hpp"

namespace haven::application::organizations {

ListOrganizationsHandler::ListOrganizationsHandler(
    OrganizationRepository& organization_repository) noexcept
    : organization_repository_(organization_repository) {}

std::vector<haven::domain::Organization> ListOrganizationsHandler::handle() const {
    return organization_repository_.find_all();
}

}  // namespace haven::application::organizations
