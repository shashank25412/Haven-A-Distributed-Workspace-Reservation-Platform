/**
 * @file cancel_reservation_handler.cpp
 * @brief Implements the CancelReservation application use case.
 */

#include "haven/application/reservations/cancel_reservation_handler.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

namespace haven::application::reservations {

CancelReservationHandler::CancelReservationHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

CancelReservationResult CancelReservationHandler::handle(
    const CancelReservationCommand& command) const {
    HVN_TRACE_SCOPE();

    auto loaded =
        reservation_repository_.find_by_id(command.organization_id(), command.reservation_id());

    if (!loaded.has_value() || loaded->aggregate().organization_id() != command.organization_id()) {
        HVN_WARN_LOG("Reservation cancellation rejected because the reservation is unavailable");
        return CancelReservationResult::rejected(CancelReservationStatus::RESERVATION_NOT_FOUND);
    }

    auto& reservation = loaded->aggregate();
    if (reservation.created_by() != command.caller_id()) {
        HVN_WARN_LOG("Reservation cancellation rejected because the caller is not authorized");
        return CancelReservationResult::rejected(CancelReservationStatus::CALLER_NOT_AUTHORIZED);
    }

    if (haven::domain::is_terminal(reservation.status())) {
        HVN_WARN_LOG("Reservation cancellation rejected because the state is terminal");
        return CancelReservationResult::rejected(CancelReservationStatus::INVALID_STATE);
    }

    reservation.cancel(command.caller_id(), command.occurred_at(), command.cancellation_event_id());

    static_cast<void>(reservation_repository_.update(
        command.organization_id(), reservation, loaded->persistence_token()));

    HVN_INFO_LOG("Reservation cancelled successfully");
    return CancelReservationResult::cancelled(std::move(reservation));
}

}  // namespace haven::application::reservations
