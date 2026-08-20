/**
 * @file get_reservation_controller.hpp
 * @brief Declares route registration for retrieving one caller-owned reservation.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/get_reservation_handler.hpp"
#include "haven/application/resources/resource_repository.hpp"

#include <memory>

namespace haven::presentation::reservations {

void register_get_reservation_route(
    std::shared_ptr<haven::application::reservations::GetReservationHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::reservations
