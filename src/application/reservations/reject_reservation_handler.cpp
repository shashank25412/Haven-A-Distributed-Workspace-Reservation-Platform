/**
 * @file reject_reservation_handler.cpp
 * @brief Implements the RejectReservation application use case.
 */

#include "haven/application/reservations/reject_reservation_handler.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

namespace haven::application::reservations {

RejectReservationHandler::RejectReservationHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

RejectReservationResult RejectReservationHandler::handle(
    const RejectReservationCommand& command) const {
    HVN_TRACE_SCOPE();

    auto reservation = reservation_repository_.find_by_id(
        command.organization_id(),
        command.reservation_id());

    if (!reservation.has_value()
        || reservation->organization_id() != command.organization_id()) {
        HVN_WARN_LOG("Reservation rejection failed because the reservation is unavailable");
        return RejectReservationResult::failed(
            RejectReservationStatus::RESERVATION_NOT_FOUND);
    }

    if (reservation->status()
        != haven::domain::ReservationStatus::PendingApproval) {
        HVN_WARN_LOG("Reservation rejection failed because the state is invalid");
        return RejectReservationResult::failed(
            RejectReservationStatus::INVALID_STATE);
    }

    reservation->reject(
        command.rejected_by(),
        command.occurred_at(),
        command.rejection_event_id());

    reservation_repository_.save(
        command.organization_id(),
        *reservation);

    HVN_INFO_LOG("Reservation rejected successfully");
    return RejectReservationResult::rejected(std::move(*reservation));
}

}  // namespace haven::application::reservations
