/**
 * @file list_all_reservations_controller.hpp
 * @brief Declares route registration for the administrative reservation listing.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/list_all_reservations_handler.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/application/users/user_directory.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_list_all_reservations_route(
    std::shared_ptr<haven::application::reservations::ListAllReservationsHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication,
    std::shared_ptr<haven::application::users::UserDirectory> user_directory);

}  // namespace haven::presentation::admin
