/**
 * @file bulk_create_resources_controller.hpp
 * @brief Declares the admin bulk resource creation HTTP route.
 *
 * Accepts a JSON array of resource definitions parsed client-side from a
 * CSV/spreadsheet so the backend needs no file-format parser.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/resources/create_resource_handler.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_bulk_create_resources_route(
    std::shared_ptr<haven::application::resources::CreateResourceHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::admin
