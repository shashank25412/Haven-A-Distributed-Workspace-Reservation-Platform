/**
 * @file idempotency_document_mapper.cpp
 * @brief Implements idempotency record and Couchbase document mappings.
 */

#include "haven/infrastructure/persistence/couchbase/idempotency_document_mapper.hpp"

#include "haven/application/reservations/create_reservation_result.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/idempotency_key.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_validator.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using CreateStatus = haven::application::reservations::CreateReservationStatus;

std::string_view creation_status_to_string(const CreateStatus status) {
    using enum CreateStatus;
    switch (status) {
        case CREATED_CONFIRMED:
            return "CREATED_CONFIRMED";
        case CREATED_PENDING_APPROVAL:
            return "CREATED_PENDING_APPROVAL";
        case RESOURCE_NOT_FOUND:
            return "RESOURCE_NOT_FOUND";
        case RESOURCE_INACTIVE:
            return "RESOURCE_INACTIVE";
        case SCHEDULE_CONFLICT:
            return "SCHEDULE_CONFLICT";
        case POLICY_REJECTED:
            return "POLICY_REJECTED";
        case IDEMPOTENCY_CONFLICT:
        case IDEMPOTENCY_IN_PROGRESS:
            break;
    }
    throw std::invalid_argument("Unsupported create reservation status");
}

CreateStatus creation_status_from_string(const std::string_view value) {
    using enum CreateStatus;
    if (value == "CREATED_CONFIRMED")
        return CREATED_CONFIRMED;
    if (value == "CREATED_PENDING_APPROVAL")
        return CREATED_PENDING_APPROVAL;
    if (value == "RESOURCE_NOT_FOUND")
        return RESOURCE_NOT_FOUND;
    if (value == "RESOURCE_INACTIVE")
        return RESOURCE_INACTIVE;
    if (value == "SCHEDULE_CONFLICT")
        return SCHEDULE_CONFLICT;
    if (value == "POLICY_REJECTED")
        return POLICY_REJECTED;
    throw std::invalid_argument("Unknown create reservation status");
}

std::optional<IdempotencyResultDocument> to_result_document(
    const std::optional<haven::application::idempotency::CreateReservationResultSnapshot>& result) {
    if (!result)
        return std::nullopt;
    IdempotencyResultDocument document{
        .creation_status = std::string{creation_status_to_string(result->creation_status())}};
    if (result->is_success()) {
        document.organization_id = result->organization_id()->value();
        document.reservation_id = result->reservation_id()->value();
        document.resource_id = result->resource_id()->value();
        document.creator_id = result->creator_id()->value();
        document.interval_start = reservation_timestamp_to_string(result->interval()->start());
        document.interval_end = reservation_timestamp_to_string(result->interval()->end());
        document.purpose = result->purpose()->value();
        document.reservation_status =
            std::string{haven::domain::to_string(*result->reservation_status())};
        document.reservation_kind =
            std::string{haven::domain::to_string(*result->reservation_kind())};
        document.initial_version = result->initial_version()->value();
        document.created_at = reservation_timestamp_to_string(*result->created_at());
    }
    return document;
}

haven::application::idempotency::CreateReservationResultSnapshot to_snapshot(
    const IdempotencyResultDocument& result) {
    const auto status = creation_status_from_string(result.creation_status);
    if (!result.reservation_id) {
        return haven::application::idempotency::CreateReservationResultSnapshot::
            permanent_rejection(status);
    }
    return haven::application::idempotency::CreateReservationResultSnapshot::successful(
        status,
        haven::domain::OrganizationId{*result.organization_id},
        haven::domain::ReservationId{*result.reservation_id},
        haven::domain::ResourceId{*result.resource_id},
        haven::domain::UserId{*result.creator_id},
        haven::domain::TimeInterval{reservation_timestamp_from_string(*result.interval_start),
                                    reservation_timestamp_from_string(*result.interval_end)},
        haven::domain::Purpose{*result.purpose},
        haven::domain::reservation_status_from_string(*result.reservation_status),
        haven::domain::reservation_kind_from_string(*result.reservation_kind),
        haven::domain::Version{*result.initial_version},
        reservation_timestamp_from_string(*result.created_at));
}

}  // namespace

IdempotencyDocument to_idempotency_document(
    const haven::application::idempotency::IdempotencyRecord& record) {
    const auto& scope = record.scope();
    const auto& ids = record.generated_identifiers();
    auto document = IdempotencyDocument{
        .schema_version = kIdempotencyDocumentSchemaVersion,
        .organization_id = scope.organization_id().value(),
        .creator_id = scope.creator_id().value(),
        .operation = std::string{idempotency_operation_to_string(scope.operation())},
        .idempotency_key = scope.key().value(),
        .fingerprint = record.fingerprint().value(),
        .status = std::string{idempotency_status_to_string(record.status())},
        .reservation_id = ids.reservation_id.value(),
        .created_event_id = ids.created_event_id.value(),
        .confirmed_event_id = ids.confirmed_event_id.value(),
        .approval_requested_event_id = ids.approval_requested_event_id.value(),
        .created_at = reservation_timestamp_to_string(record.created_at()),
        .result = to_result_document(record.result()),
    };
    validate_idempotency_document(document);
    return document;
}

haven::application::idempotency::IdempotencyRecord to_idempotency_record(
    const IdempotencyDocument& document) {
    validate_idempotency_document(document);
    using namespace haven::application::idempotency;
    auto scope = IdempotencyScope{haven::domain::OrganizationId{document.organization_id},
                                  haven::domain::UserId{document.creator_id},
                                  idempotency_operation_from_string(document.operation),
                                  haven::domain::IdempotencyKey{document.idempotency_key}};
    auto ids = ReservationCreationIdentifiers{
        haven::domain::ReservationId{document.reservation_id},
        haven::domain::EventId{document.created_event_id},
        haven::domain::EventId{document.confirmed_event_id},
        haven::domain::EventId{document.approval_requested_event_id}};
    auto fingerprint = IdempotencyFingerprint{document.fingerprint};
    const auto created_at = reservation_timestamp_from_string(document.created_at);
    switch (idempotency_status_from_string(document.status)) {
        case IdempotencyStatus::Processing:
            return IdempotencyRecord::processing(
                std::move(scope), std::move(fingerprint), std::move(ids), created_at);
        case IdempotencyStatus::Succeeded:
            return IdempotencyRecord::succeeded(std::move(scope),
                                                std::move(fingerprint),
                                                std::move(ids),
                                                created_at,
                                                to_snapshot(*document.result));
        case IdempotencyStatus::FailedPermanent:
            return IdempotencyRecord::failed_permanently(std::move(scope),
                                                         std::move(fingerprint),
                                                         std::move(ids),
                                                         created_at,
                                                         to_snapshot(*document.result));
    }
    throw std::invalid_argument("Unsupported idempotency status");
}

}  // namespace haven::infrastructure::persistence::couchbase
