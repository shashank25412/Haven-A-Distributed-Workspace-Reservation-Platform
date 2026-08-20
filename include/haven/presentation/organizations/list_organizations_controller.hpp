/**
 * @file list_organizations_controller.hpp
 * @brief Declares manual registration for the organization directory listing endpoint.
 */

#pragma once

#include "haven/application/organizations/list_organizations_handler.hpp"

#include <memory>

namespace haven::presentation::organizations {

/**
 * @brief Registers the organization directory listing route.
 *
 * @param handler Application handler retained for the route callback lifetime.
 */
void register_list_organizations_route(
    std::shared_ptr<haven::application::organizations::ListOrganizationsHandler> handler);

}  // namespace haven::presentation::organizations
