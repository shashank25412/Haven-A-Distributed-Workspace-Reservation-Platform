/**
 * @file list_all_reservations_handler.cpp
 * @brief Implements the ListAllReservations application use case.
 */

#include "haven/application/reservations/list_all_reservations_handler.hpp"

#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::reservations {

ListAllReservationsHandler::ListAllReservationsHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

ReservationListResult ListAllReservationsHandler::handle(
    const ListAllReservationsQuery& query) const {
    HVN_TRACE_SCOPE();

    auto reservations = reservation_repository_.find_all(query.organization_id());

    const auto first_hidden_reservation = std::remove_if(
        reservations.begin(),
        reservations.end(),
        [&query](const haven::domain::Reservation& reservation) {
            return reservation.organization_id() != query.organization_id();
        });

    if (first_hidden_reservation != reservations.end()) {
        HVN_WARN_LOG("Reservation repository returned reservations outside the requested organization");
        reservations.erase(first_hidden_reservation, reservations.end());
    }

    return reservations;
}

}  // namespace haven::application::reservations
