/**
 * @file idempotency_document_validator.cpp
 * @brief Implements validation for persisted idempotency documents.
 */

#include "haven/infrastructure/persistence/couchbase/idempotency_document_validator.hpp"

#include "haven/application/reservations/create_reservation_result.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {
namespace {

void require_non_empty(const std::string& value, const char* field) {
    if (value.empty()) {
        throw std::invalid_argument(std::string{"Idempotency document field is empty: "} + field);
    }
}

bool successful_status(const std::string_view value) {
    return value == "CREATED_CONFIRMED" || value == "CREATED_PENDING_APPROVAL";
}

bool rejection_status(const std::string_view value) {
    return value == "RESOURCE_NOT_FOUND" || value == "RESOURCE_INACTIVE" ||
           value == "SCHEDULE_CONFLICT" || value == "POLICY_REJECTED";
}

}  // namespace

std::string_view idempotency_operation_to_string(
    const haven::application::idempotency::IdempotencyOperation operation) {
    using enum haven::application::idempotency::IdempotencyOperation;
    switch (operation) {
        case CreateReservation:
            return "CREATE_RESERVATION";
        case ExtendReservation:
            return "EXTEND_RESERVATION";
    }
    throw std::invalid_argument("Unsupported idempotency operation");
}

haven::application::idempotency::IdempotencyOperation idempotency_operation_from_string(
    const std::string_view value) {
    using haven::application::idempotency::IdempotencyOperation;
    if (value == "CREATE_RESERVATION")
        return IdempotencyOperation::CreateReservation;
    if (value == "EXTEND_RESERVATION")
        return IdempotencyOperation::ExtendReservation;
    throw std::invalid_argument("Unknown idempotency operation");
}

std::string_view idempotency_status_to_string(
    const haven::application::idempotency::IdempotencyStatus status) {
    using enum haven::application::idempotency::IdempotencyStatus;
    switch (status) {
        case Processing:
            return "PROCESSING";
        case Succeeded:
            return "SUCCEEDED";
        case FailedPermanent:
            return "FAILED_PERMANENT";
    }
    throw std::invalid_argument("Unsupported idempotency status");
}

haven::application::idempotency::IdempotencyStatus idempotency_status_from_string(
    const std::string_view value) {
    using haven::application::idempotency::IdempotencyStatus;
    if (value == "PROCESSING")
        return IdempotencyStatus::Processing;
    if (value == "SUCCEEDED")
        return IdempotencyStatus::Succeeded;
    if (value == "FAILED_PERMANENT")
        return IdempotencyStatus::FailedPermanent;
    throw std::invalid_argument("Unknown idempotency status");
}

void validate_idempotency_document(const IdempotencyDocument& document) {
    if (document.schema_version != kIdempotencyDocumentSchemaVersion) {
        throw std::invalid_argument("Unsupported idempotency document schema version");
    }
    require_non_empty(document.organization_id, "organizationId");
    require_non_empty(document.creator_id, "creatorId");
    require_non_empty(document.idempotency_key, "idempotencyKey");
    require_non_empty(document.reservation_id, "reservationId");
    require_non_empty(document.created_event_id, "createdEventId");
    require_non_empty(document.confirmed_event_id, "confirmedEventId");
    require_non_empty(document.approval_requested_event_id, "approvalRequestedEventId");
    static_cast<void>(idempotency_operation_from_string(document.operation));
    const auto status = idempotency_status_from_string(document.status);
    if (document.fingerprint.size() != 64 ||
        !std::all_of(document.fingerprint.begin(), document.fingerprint.end(), [](const char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        })) {
        throw std::invalid_argument("Idempotency fingerprint is not lowercase SHA-256 hex");
    }
    static_cast<void>(reservation_timestamp_from_string(document.created_at));
    using enum haven::application::idempotency::IdempotencyStatus;
    if (status == Processing) {
        if (document.result.has_value())
            throw std::invalid_argument("Processing record has result");
        return;
    }
    if (!document.result.has_value())
        throw std::invalid_argument("Terminal record lacks result");
    const auto& result = *document.result;
    const bool all_success_fields =
        result.organization_id && result.reservation_id && result.resource_id &&
        result.creator_id && result.interval_start && result.interval_end && result.purpose &&
        result.reservation_status && result.reservation_kind && result.initial_version &&
        result.created_at;
    const bool any_success_fields =
        result.organization_id || result.reservation_id || result.resource_id ||
        result.creator_id || result.interval_start || result.interval_end || result.purpose ||
        result.reservation_status || result.reservation_kind || result.initial_version ||
        result.created_at;
    if (status == Succeeded) {
        if (!successful_status(result.creation_status) || !all_success_fields ||
            *result.organization_id != document.organization_id ||
            *result.reservation_id != document.reservation_id ||
            *result.creator_id != document.creator_id || *result.initial_version == 0) {
            throw std::invalid_argument("Invalid successful idempotency result");
        }
        static_cast<void>(
            haven::domain::reservation_status_from_string(*result.reservation_status));
        static_cast<void>(haven::domain::reservation_kind_from_string(*result.reservation_kind));
        static_cast<void>(reservation_timestamp_from_string(*result.created_at));
        const auto start = reservation_timestamp_from_string(*result.interval_start);
        const auto end = reservation_timestamp_from_string(*result.interval_end);
        if (start >= end)
            throw std::invalid_argument("Successful result interval is invalid");
        if (*result.created_at != document.created_at)
            throw std::invalid_argument("Successful result creation time does not match record");
    } else if (!rejection_status(result.creation_status) || any_success_fields) {
        throw std::invalid_argument("Invalid permanent rejection result");
    }
}

}  // namespace haven::infrastructure::persistence::couchbase
