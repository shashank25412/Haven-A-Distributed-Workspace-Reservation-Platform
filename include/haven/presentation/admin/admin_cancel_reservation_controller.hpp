/**
 * @file admin_cancel_reservation_controller.hpp
 * @brief Declares route registration for administrative reservation cancellation.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/cancel_reservation_handler.hpp"

#include <memory>

namespace haven::presentation::admin {

void register_admin_cancel_reservation_route(
    std::shared_ptr<haven::application::reservations::CancelReservationHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::admin
