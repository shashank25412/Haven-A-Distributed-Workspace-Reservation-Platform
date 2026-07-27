/**
 * @file get_reservation_handler.cpp
 * @brief Implements the GetReservation application use case.
 */

#include "haven/application/reservations/get_reservation_handler.hpp"

#include "haven/logging/logging.hpp"

namespace haven::application::reservations {

GetReservationHandler::GetReservationHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

ReservationLookupResult GetReservationHandler::handle(
    const GetReservationQuery& query) const {
    HVN_TRACE_SCOPE();

    auto reservation = reservation_repository_.find_by_id(
        query.organization_id(),
        query.reservation_id());

    if (!reservation.has_value()) {
        return std::nullopt;
    }

    if (reservation->organization_id() != query.organization_id()) {
        HVN_WARN_LOG(
            "Reservation repository returned a reservation outside the requested tenant scope");
        return std::nullopt;
    }

    return reservation;
}

}  // namespace haven::application::reservations