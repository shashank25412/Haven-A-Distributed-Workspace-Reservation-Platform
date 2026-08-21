/**
 * @file update_resource_controller.hpp
 * @brief Declares the admin resource update HTTP route.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/resources/update_resource_handler.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_update_resource_route(
    std::shared_ptr<haven::application::resources::UpdateResourceHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::admin
