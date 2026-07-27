/**
 * @file create_reservation_handler.cpp
 * @brief Implements the CreateReservation application use case.
 */

#include "haven/application/reservations/create_reservation_handler.hpp"

#include "haven/logging/logging.hpp"

namespace haven::application::reservations {

CreateReservationHandler::CreateReservationHandler(
    haven::application::resources::ResourceRepository& resource_repository,
    ReservationRepository& reservation_repository,
    const haven::domain::ReservationCreationPolicy& reservation_creation_policy) noexcept
    : resource_repository_(resource_repository),
      reservation_repository_(reservation_repository),
      reservation_creation_policy_(reservation_creation_policy) {}

CreateReservationResult CreateReservationHandler::handle(
    const CreateReservationCommand& command) const {
    HVN_TRACE_SCOPE();

    const auto resource = resource_repository_.find_by_id(
        command.organization_id(),
        command.resource_id());

    if (!resource.has_value()
        || resource->organization_id() != command.organization_id()) {
        HVN_WARN_LOG("Reservation creation rejected because the resource is unavailable");
        return CreateReservationResult::rejected(
            CreateReservationStatus::RESOURCE_NOT_FOUND);
    }

    if (!resource->is_active()) {
        HVN_WARN_LOG("Reservation creation rejected because the resource is inactive");
        return CreateReservationResult::rejected(
            CreateReservationStatus::RESOURCE_INACTIVE);
    }

    const auto policy_result = reservation_creation_policy_.evaluate(
        *resource,
        command.interval(),
        command.reservation_kind(),
        command.maintenance_authorized());

    if (policy_result != haven::domain::ReservationCreationDecision::Confirmed
        && policy_result != haven::domain::ReservationCreationDecision::PendingApproval) {
        HVN_WARN_LOG("Reservation creation rejected by domain policy");
        return CreateReservationResult::rejected(
            CreateReservationStatus::POLICY_REJECTED);
    }

    if (reservation_repository_.has_conflict(
            command.organization_id(),
            command.resource_id(),
            command.interval())) {
        HVN_WARN_LOG("Reservation creation rejected because the schedule conflicts");
        return CreateReservationResult::rejected(
            CreateReservationStatus::SCHEDULE_CONFLICT);
    }

    if (resource->requires_approval()) {
        auto reservation = haven::domain::Reservation::create_pending_approval(
            command.organization_id(),
            command.reservation_id(),
            command.resource_id(),
            command.creator_id(),
            command.interval(),
            command.purpose(),
            command.reservation_kind(),
            command.created_event_id(),
            command.approval_requested_event_id(),
            command.occurred_at());

        reservation_repository_.save(command.organization_id(), reservation);

        HVN_INFO_LOG("Pending reservation created successfully");
        return CreateReservationResult::pending_approval(std::move(reservation));
    }

    auto reservation = haven::domain::Reservation::create_confirmed(
        command.organization_id(),
        command.reservation_id(),
        command.resource_id(),
        command.creator_id(),
        command.interval(),
        command.purpose(),
        command.reservation_kind(),
        command.created_event_id(),
        command.confirmed_event_id(),
        command.occurred_at());

    reservation_repository_.save(command.organization_id(), reservation);

    HVN_INFO_LOG("Confirmed reservation created successfully");
    return CreateReservationResult::confirmed(std::move(reservation));
}

}  // namespace haven::application::reservations
