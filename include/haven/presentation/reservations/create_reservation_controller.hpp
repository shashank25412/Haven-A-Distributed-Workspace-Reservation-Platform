/**
 * @file create_reservation_controller.hpp
 * @brief Declares route registration for reservation creation.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/create_reservation_handler.hpp"

#include <memory>

namespace haven::presentation::reservations {

void register_create_reservation_route(
    std::shared_ptr<haven::application::reservations::CreateReservationHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::reservations
