/**
 * @file list_admin_resources_controller.hpp
 * @brief Declares the admin resource listing HTTP route.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/resources/resource_repository.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_list_admin_resources_route(
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::admin
