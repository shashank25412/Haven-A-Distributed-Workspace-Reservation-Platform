/**
 * @file list_caller_reservations_handler.cpp
 * @brief Implements the ListCallerReservations application use case.
 */

#include "haven/application/reservations/list_caller_reservations_handler.hpp"

#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::reservations {

ListCallerReservationsHandler::ListCallerReservationsHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

ReservationListResult ListCallerReservationsHandler::handle(
    const ListCallerReservationsQuery& query) const {
    HVN_TRACE_SCOPE();

    auto reservations = reservation_repository_.find_by_creator(
        query.organization_id(),
        query.caller_id());

    const auto first_hidden_reservation = std::remove_if(
        reservations.begin(),
        reservations.end(),
        [&query](const haven::domain::Reservation& reservation) {
            return reservation.organization_id() != query.organization_id()
                || reservation.created_by() != query.caller_id();
        });

    if (first_hidden_reservation != reservations.end()) {
        HVN_WARN_LOG(
            "Reservation repository returned reservations outside the requested caller scope");
        reservations.erase(first_hidden_reservation, reservations.end());
    }

    return reservations;
}

}  // namespace haven::application::reservations
