/**
 * @file approve_reservation_controller.hpp
 * @brief Declares route registration for reservation approval.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/approve_reservation_handler.hpp"

#include <memory>

namespace haven::presentation::approvals {

void register_approve_reservation_route(
    std::shared_ptr<haven::application::reservations::ApproveReservationHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::approvals
