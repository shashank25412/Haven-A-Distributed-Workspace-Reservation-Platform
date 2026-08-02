/**
 * @file outbox_document_validator.cpp
 * @brief Implements validation for persisted Outbox documents.
 */

#include "haven/infrastructure/persistence/couchbase/outbox_document_validator.hpp"

#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] const std::string& required_string(const tao::json::value& payload,
                                                 const char* name) {
    const auto& value = payload.at(name).get_string();
    if (value.empty())
        throw std::invalid_argument(std::string{"Outbox payload field is empty: "} + name);
    return value;
}

[[nodiscard]] haven::domain::TimeInterval required_interval(const tao::json::value& payload,
                                                            const char* start_name,
                                                            const char* end_name) {
    return haven::domain::TimeInterval{
        reservation_timestamp_from_string(required_string(payload, start_name)),
        reservation_timestamp_from_string(required_string(payload, end_name))};
}

void validate_created_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_string(payload, "createdBy"));
    static_cast<void>(required_interval(payload, "startTime", "endTime"));
    static_cast<void>(
        haven::domain::reservation_kind_from_string(required_string(payload, "kind")));
    const auto status =
        haven::domain::reservation_status_from_string(required_string(payload, "initialStatus"));
    if (status != haven::domain::ReservationStatus::Confirmed &&
        status != haven::domain::ReservationStatus::PendingApproval) {
        throw std::invalid_argument("Outbox creation payload has invalid initial status");
    }
}

void validate_confirmed_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_interval(payload, "startTime", "endTime"));
    if (payload.get_object().contains("confirmedBy"))
        static_cast<void>(required_string(payload, "confirmedBy"));
}

void validate_approval_requested_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_string(payload, "requestedBy"));
    static_cast<void>(required_interval(payload, "startTime", "endTime"));
    static_cast<void>(
        haven::domain::reservation_kind_from_string(required_string(payload, "kind")));
}

void validate_rejected_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_string(payload, "rejectedBy"));
}

void validate_cancelled_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_string(payload, "cancelledBy"));
    const auto status =
        haven::domain::reservation_status_from_string(required_string(payload, "previousStatus"));
    if (status != haven::domain::ReservationStatus::Confirmed &&
        status != haven::domain::ReservationStatus::PendingApproval) {
        throw std::invalid_argument("Outbox cancellation payload has invalid previous status");
    }
}

void validate_extended_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_string(payload, "extendedBy"));
    const auto previous = required_interval(payload, "previousStartTime", "previousEndTime");
    const auto extended = required_interval(payload, "extendedStartTime", "extendedEndTime");
    if (previous.start() != extended.start() || previous.end() >= extended.end())
        throw std::invalid_argument("Outbox extension payload has inconsistent intervals");
}

void validate_expired_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    const auto status =
        haven::domain::reservation_status_from_string(required_string(payload, "previousStatus"));
    if (status != haven::domain::ReservationStatus::Confirmed &&
        status != haven::domain::ReservationStatus::PendingApproval) {
        throw std::invalid_argument("Outbox expiration payload has invalid previous status");
    }
}

void validate_completed_payload(const tao::json::value& payload) {
    static_cast<void>(required_string(payload, "resourceId"));
    static_cast<void>(required_interval(payload, "startTime", "endTime"));
}

void validate_payload(const std::string_view event_type, const tao::json::value& payload) {
    static_cast<void>(payload.get_object());
    if (event_type == kReservationCreatedEventType)
        return validate_created_payload(payload);
    if (event_type == kReservationConfirmedEventType)
        return validate_confirmed_payload(payload);
    if (event_type == kReservationApprovalRequestedEventType)
        return validate_approval_requested_payload(payload);
    if (event_type == kReservationRejectedEventType)
        return validate_rejected_payload(payload);
    if (event_type == kReservationCancelledEventType)
        return validate_cancelled_payload(payload);
    if (event_type == kReservationExtendedEventType)
        return validate_extended_payload(payload);
    if (event_type == kReservationExpiredEventType)
        return validate_expired_payload(payload);
    if (event_type == kReservationCompletedEventType)
        return validate_completed_payload(payload);
    throw std::invalid_argument("Unknown Outbox event type");
}

}  // namespace

void validate_outbox_document(const OutboxDocument& document) {
    if (document.schema_version != kOutboxDocumentSchemaVersion)
        throw std::invalid_argument("Unsupported Couchbase Outbox document schema version");
    if (document.aggregate_type != kReservationAggregateType)
        throw std::invalid_argument("Unexpected Outbox aggregate type");
    static_cast<void>(to_string(document.status));
    validate_payload(document.event_type, document.payload);

    if (document.status == OutboxStatus::Published && !document.published_at.has_value()) {
        throw std::invalid_argument("Published Outbox document requires publishedAt");
    }
    if (document.status != OutboxStatus::Published && document.published_at.has_value()) {
        throw std::invalid_argument("Unpublished Outbox document must not contain publishedAt");
    }
}

}  // namespace haven::infrastructure::persistence::couchbase
