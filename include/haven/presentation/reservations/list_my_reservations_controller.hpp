/**
 * @file list_my_reservations_controller.hpp
 * @brief Declares route registration for listing the caller's reservations.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/list_caller_reservations_handler.hpp"
#include "haven/application/resources/resource_repository.hpp"

#include <memory>

namespace haven::presentation::reservations {

void register_list_my_reservations_route(
    std::shared_ptr<haven::application::reservations::ListCallerReservationsHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::reservations
