/**
 * @file create_reservation_handler.cpp
 * @brief Implements idempotent reservation creation orchestration.
 */

#include "haven/application/reservations/create_reservation_handler.hpp"

#include "haven/application/idempotency/create_reservation_fingerprint_input.hpp"
#include "haven/application/idempotency/idempotency_record.hpp"
#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::reservations {
namespace {

using namespace haven::application::idempotency;

CreateReservationResult replay(const CreateReservationResultSnapshot& snapshot) {
    if (!snapshot.is_success()) {
        return CreateReservationResult::rejected(snapshot.creation_status());
    }
    auto reservation = haven::domain::Reservation::rehydrate(*snapshot.organization_id(),
                                                             *snapshot.reservation_id(),
                                                             *snapshot.resource_id(),
                                                             *snapshot.creator_id(),
                                                             *snapshot.interval(),
                                                             *snapshot.purpose(),
                                                             *snapshot.reservation_kind(),
                                                             *snapshot.reservation_status(),
                                                             std::nullopt,
                                                             *snapshot.initial_version());
    if (snapshot.creation_status() == CreateReservationStatus::CREATED_CONFIRMED) {
        return CreateReservationResult::confirmed(std::move(reservation));
    }
    return CreateReservationResult::pending_approval(std::move(reservation));
}

CreateReservationResultSnapshot snapshot_for(const CreateReservationResult& result,
                                             const IdempotencyRecord::TimePoint created_at) {
    if (!result.reservation()) {
        return CreateReservationResultSnapshot::permanent_rejection(result.status());
    }
    const auto& reservation = *result.reservation();
    return CreateReservationResultSnapshot::successful(result.status(),
                                                       reservation.organization_id(),
                                                       reservation.reservation_id(),
                                                       reservation.resource_id(),
                                                       reservation.created_by(),
                                                       reservation.interval(),
                                                       reservation.purpose(),
                                                       reservation.status(),
                                                       reservation.kind(),
                                                       reservation.version(),
                                                       created_at);
}

bool matches_original_operation(const haven::domain::Reservation& reservation,
                                const IdempotencyRecord& record,
                                const CreateReservationCommand& command) {
    const auto creation_compatible =
        reservation.status() == haven::domain::ReservationStatus::PendingApproval ||
        (reservation.status() == haven::domain::ReservationStatus::Confirmed &&
         !reservation.approval_info().has_value());
    return creation_compatible &&
           reservation.organization_id() == record.scope().organization_id() &&
           reservation.reservation_id() == record.generated_identifiers().reservation_id &&
           reservation.resource_id() == command.resource_id() &&
           reservation.created_by() == command.creator_id() &&
           reservation.interval() == command.interval() &&
           reservation.purpose() == command.purpose() &&
           reservation.kind() == command.reservation_kind();
}

CreateReservationResult result_for_recovered(haven::domain::Reservation reservation) {
    if (reservation.status() == haven::domain::ReservationStatus::Confirmed) {
        return CreateReservationResult::confirmed(std::move(reservation));
    }
    return CreateReservationResult::pending_approval(std::move(reservation));
}

}  // namespace

CreateReservationHandler::CreateReservationHandler(
    haven::application::resources::ResourceRepository& resource_repository,
    ReservationRepository& reservation_repository,
    IdempotencyRepository& idempotency_repository,
    const haven::domain::ReservationCreationPolicy& reservation_creation_policy) noexcept
    : resource_repository_(resource_repository),
      reservation_repository_(reservation_repository),
      idempotency_repository_(idempotency_repository),
      reservation_creation_policy_(reservation_creation_policy) {}

CreateReservationResult CreateReservationHandler::handle(
    const CreateReservationCommand& command) const {
    HVN_TRACE_SCOPE();
    const auto scope = IdempotencyScope{command.organization_id(),
                                        command.creator_id(),
                                        IdempotencyOperation::CreateReservation,
                                        command.idempotency_key()};
    const auto fingerprint = create_reservation_fingerprint(
        CreateReservationFingerprintInput{command.resource_id(),
                                          command.creator_id(),
                                          command.interval(),
                                          command.purpose(),
                                          command.reservation_kind(),
                                          command.maintenance_authorized()});
    const auto processing_record = IdempotencyRecord::processing(
        scope,
        fingerprint,
        ReservationCreationIdentifiers{command.reservation_id(),
                                       command.created_event_id(),
                                       command.confirmed_event_id(),
                                       command.approval_requested_event_id()},
        command.occurred_at());
    const auto claim = idempotency_repository_.claim(processing_record);
    switch (claim.status()) {
        case IdempotencyClaimStatus::ExistingProcessing:
            HVN_DEBUG_LOG("Reservation creation processing record found; attempting recovery");
            if (const auto loaded = reservation_repository_.find_by_id(
                    claim.record().scope().organization_id(),
                    claim.record().generated_identifiers().reservation_id)) {
                if (!matches_original_operation(loaded->aggregate(), claim.record(), command)) {
                    HVN_WARN_LOG("Reservation creation recovery identity mismatch");
                    return CreateReservationResult::rejected(
                        CreateReservationStatus::IDEMPOTENCY_CONFLICT);
                }
                auto recovered = result_for_recovered(loaded->aggregate());
                idempotency_repository_.record_succeeded(
                    claim.record().scope(),
                    claim.record().fingerprint(),
                    snapshot_for(recovered, claim.record().created_at()));
                HVN_DEBUG_LOG("Reservation creation recovery completion succeeded");
                return recovered;
            }
            HVN_DEBUG_LOG("Reservation creation processing record remains active");
            return CreateReservationResult::rejected(
                CreateReservationStatus::IDEMPOTENCY_IN_PROGRESS);
        case IdempotencyClaimStatus::FingerprintMismatch:
            HVN_WARN_LOG(
                "Reservation creation idempotency request conflicts with its original payload");
            return CreateReservationResult::rejected(CreateReservationStatus::IDEMPOTENCY_CONFLICT);
        case IdempotencyClaimStatus::ExistingSucceeded:
        case IdempotencyClaimStatus::ExistingFailedPermanent:
            if (!claim.record().result()) {
                throw std::logic_error(
                    "Terminal idempotency record is missing its result snapshot");
            }
            HVN_DEBUG_LOG("Replaying completed reservation creation result");
            return replay(*claim.record().result());
        case IdempotencyClaimStatus::Claimed:
            HVN_DEBUG_LOG("Reservation creation idempotency claim acquired");
            break;
    }

    const auto reject = [&](const CreateReservationStatus status) {
        auto result = CreateReservationResult::rejected(status);
        idempotency_repository_.record_failed_permanently(
            scope, fingerprint, snapshot_for(result, command.occurred_at()));
        return result;
    };

    const auto resource =
        resource_repository_.find_by_id(command.organization_id(), command.resource_id());
    if (!resource || resource->aggregate().organization_id() != command.organization_id()) {
        HVN_WARN_LOG("Reservation creation rejected because the resource is unavailable");
        return reject(CreateReservationStatus::RESOURCE_NOT_FOUND);
    }
    const auto& aggregate = resource->aggregate();
    if (!aggregate.is_active()) {
        HVN_WARN_LOG("Reservation creation rejected because the resource is inactive");
        return reject(CreateReservationStatus::RESOURCE_INACTIVE);
    }
    const auto policy_result =
        reservation_creation_policy_.evaluate(aggregate,
                                              command.interval(),
                                              command.reservation_kind(),
                                              command.maintenance_authorized());
    if (policy_result != haven::domain::ReservationCreationDecision::Confirmed &&
        policy_result != haven::domain::ReservationCreationDecision::PendingApproval) {
        HVN_WARN_LOG("Reservation creation rejected by domain policy");
        return reject(CreateReservationStatus::POLICY_REJECTED);
    }
    if (reservation_repository_.has_conflict(
            command.organization_id(), command.resource_id(), command.interval())) {
        HVN_WARN_LOG("Reservation creation rejected because the schedule conflicts");
        return reject(CreateReservationStatus::SCHEDULE_CONFLICT);
    }

    auto reservation =
        aggregate.requires_approval()
            ? haven::domain::Reservation::create_pending_approval(
                  command.organization_id(),
                  command.reservation_id(),
                  command.resource_id(),
                  command.creator_id(),
                  command.interval(),
                  command.purpose(),
                  command.reservation_kind(),
                  command.created_event_id(),
                  command.approval_requested_event_id(),
                  command.occurred_at())
            : haven::domain::Reservation::create_confirmed(command.organization_id(),
                                                           command.reservation_id(),
                                                           command.resource_id(),
                                                           command.creator_id(),
                                                           command.interval(),
                                                           command.purpose(),
                                                           command.reservation_kind(),
                                                           command.created_event_id(),
                                                           command.confirmed_event_id(),
                                                           command.occurred_at());
    static_cast<void>(reservation_repository_.insert(command.organization_id(), reservation));
    auto result = aggregate.requires_approval()
                      ? CreateReservationResult::pending_approval(std::move(reservation))
                      : CreateReservationResult::confirmed(std::move(reservation));
    // If completion fails, the inserted reservation remains durable while the
    // claim remains Processing. Recovery is deliberately deferred to a later slice.
    idempotency_repository_.record_succeeded(
        scope, fingerprint, snapshot_for(result, command.occurred_at()));
    return result;
}

}  // namespace haven::application::reservations
