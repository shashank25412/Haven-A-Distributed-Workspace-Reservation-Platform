/**
 * @file extend_reservation_handler.cpp
 * @brief Implements the ExtendReservation application use case.
 */

#include "haven/application/reservations/extend_reservation_handler.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

namespace haven::application::reservations {

ExtendReservationHandler::ExtendReservationHandler(
    ReservationRepository& reservation_repository,
    const haven::domain::ReservationPolicy& reservation_policy) noexcept
    : reservation_repository_(reservation_repository), reservation_policy_(reservation_policy) {}

ExtendReservationResult ExtendReservationHandler::handle(
    const ExtendReservationCommand& command) const {
    HVN_TRACE_SCOPE();

    auto loaded =
        reservation_repository_.find_by_id(command.organization_id(), command.reservation_id());

    if (!loaded.has_value() || loaded->aggregate().organization_id() != command.organization_id()) {
        HVN_WARN_LOG("Reservation extension rejected because the reservation is unavailable");
        return ExtendReservationResult::rejected(ExtendReservationStatus::RESERVATION_NOT_FOUND);
    }

    auto& reservation = loaded->aggregate();
    if (reservation.created_by() != command.caller_id()) {
        HVN_WARN_LOG("Reservation extension rejected because the caller is not authorized");
        return ExtendReservationResult::rejected(ExtendReservationStatus::CALLER_NOT_AUTHORIZED);
    }

    if (reservation.status() != haven::domain::ReservationStatus::Confirmed) {
        HVN_WARN_LOG("Reservation extension rejected because the state is not extendable");
        return ExtendReservationResult::rejected(ExtendReservationStatus::INVALID_STATE);
    }

    if (command.interval().start() != reservation.interval().start() ||
        command.interval().end() <= reservation.interval().end()) {
        HVN_WARN_LOG("Reservation extension rejected because the interval is not an extension");
        return ExtendReservationResult::rejected(ExtendReservationStatus::POLICY_REJECTED);
    }

    const auto policy_result =
        reservation_policy_.evaluate(command.interval(), reservation.kind(), true);

    if (policy_result != haven::domain::ReservationPolicyViolation::None) {
        HVN_WARN_LOG("Reservation extension rejected by interval policy");
        return ExtendReservationResult::rejected(ExtendReservationStatus::POLICY_REJECTED);
    }

    if (reservation_repository_.has_conflict_excluding(command.organization_id(),
                                                       reservation.resource_id(),
                                                       command.interval(),
                                                       command.reservation_id())) {
        HVN_WARN_LOG("Reservation extension rejected because the schedule conflicts");
        return ExtendReservationResult::rejected(ExtendReservationStatus::SCHEDULE_CONFLICT);
    }

    reservation.extend(command.interval().end(),
                       command.caller_id(),
                       command.occurred_at(),
                       command.extension_event_id());

    static_cast<void>(reservation_repository_.update(
        command.organization_id(), reservation, loaded->persistence_token()));

    HVN_INFO_LOG("Reservation extended successfully");
    return ExtendReservationResult::extended(std::move(reservation));
}

}  // namespace haven::application::reservations
