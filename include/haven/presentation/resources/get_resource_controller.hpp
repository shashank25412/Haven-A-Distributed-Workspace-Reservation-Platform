/**
 * @file get_resource_controller.hpp
 * @brief Declares manual registration for the Resource detail endpoint.
 */

#pragma once

#include "haven/application/resources/get_resource_handler.hpp"

#include <memory>
#include <string>

namespace haven::presentation::resources {

/**
 * @brief Registers the tenant-scoped Resource detail route.
 *
 * @param handler Application handler retained for the route callback lifetime.
 */
void register_get_resource_route(
    std::shared_ptr<haven::application::resources::GetResourceHandler> handler,
    std::string public_organization_id);

}  // namespace haven::presentation::resources
