/**
 * @file approve_reservation_handler.cpp
 * @brief Implements the ApproveReservation application use case.
 */

#include "haven/application/reservations/approve_reservation_handler.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

namespace haven::application::reservations {

ApproveReservationHandler::ApproveReservationHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

ApproveReservationResult ApproveReservationHandler::handle(
    const ApproveReservationCommand& command) const {
    HVN_TRACE_SCOPE();

    auto loaded =
        reservation_repository_.find_by_id(command.organization_id(), command.reservation_id());

    if (!loaded.has_value() || loaded->aggregate().organization_id() != command.organization_id()) {
        HVN_WARN_LOG("Reservation approval rejected because the reservation is unavailable");
        return ApproveReservationResult::rejected(ApproveReservationStatus::RESERVATION_NOT_FOUND);
    }

    auto& reservation = loaded->aggregate();
    if (reservation.status() != haven::domain::ReservationStatus::PendingApproval) {
        HVN_WARN_LOG("Reservation approval rejected because the state is invalid");
        return ApproveReservationResult::rejected(ApproveReservationStatus::INVALID_STATE);
    }

    if (reservation_repository_.has_conflict_excluding(command.organization_id(),
                                                       reservation.resource_id(),
                                                       reservation.interval(),
                                                       command.reservation_id())) {
        HVN_WARN_LOG("Reservation approval rejected because the schedule conflicts");
        return ApproveReservationResult::rejected(ApproveReservationStatus::SCHEDULE_CONFLICT);
    }

    reservation.approve(command.approver_id(), command.occurred_at(), command.approval_event_id());

    static_cast<void>(reservation_repository_.update(
        command.organization_id(), reservation, loaded->persistence_token()));

    HVN_INFO_LOG("Reservation approved successfully");
    return ApproveReservationResult::approved(std::move(reservation));
}

}  // namespace haven::application::reservations
