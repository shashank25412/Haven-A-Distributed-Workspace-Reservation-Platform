/**
 * @file create_resource_controller.hpp
 * @brief Declares the admin resource creation HTTP route.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/resources/create_resource_handler.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_create_resource_route(
    std::shared_ptr<haven::application::resources::CreateResourceHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::admin
