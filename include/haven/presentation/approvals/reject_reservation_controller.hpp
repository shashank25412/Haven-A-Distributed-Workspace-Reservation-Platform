/**
 * @file reject_reservation_controller.hpp
 * @brief Declares route registration for reservation rejection.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/reject_reservation_handler.hpp"

#include <memory>

namespace haven::presentation::approvals {

void register_reject_reservation_route(
    std::shared_ptr<haven::application::reservations::RejectReservationHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::approvals
