/**
 * @file delete_resource_controller.hpp
 * @brief Declares the admin resource deletion HTTP route.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/resources/resource_repository.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_delete_resource_route(
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::admin
