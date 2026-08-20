/**
 * @file outbox_document_mapper.cpp
 * @brief Implements Reservation domain-event to Outbox document mapping.
 */

#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"

#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_validator.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] tao::json::value interval_payload(const haven::domain::TimeInterval& interval) {
    return tao::json::value{{"startTime", reservation_timestamp_to_string(interval.start())},
                            {"endTime", reservation_timestamp_to_string(interval.end())}};
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationCreatedEvent& event) {
    auto payload = interval_payload(event.interval());
    payload["resourceId"] = event.resource_id().value();
    payload["createdBy"] = event.created_by().value();
    payload["kind"] = std::string{haven::domain::to_string(event.kind())};
    payload["initialStatus"] = std::string{haven::domain::to_string(event.initial_status())};
    return payload;
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationConfirmedEvent& event) {
    auto payload = interval_payload(event.interval());
    payload["resourceId"] = event.resource_id().value();
    if (event.confirmed_by().has_value())
        payload["confirmedBy"] = event.confirmed_by()->value();
    return payload;
}

[[nodiscard]] tao::json::value payload_for(
    const haven::domain::ReservationApprovalRequestedEvent& event) {
    auto payload = interval_payload(event.interval());
    payload["resourceId"] = event.resource_id().value();
    payload["requestedBy"] = event.requested_by().value();
    payload["kind"] = std::string{haven::domain::to_string(event.kind())};
    return payload;
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationRejectedEvent& event) {
    return tao::json::value{{"resourceId", event.resource_id().value()},
                            {"rejectedBy", event.rejected_by().value()}};
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationCancelledEvent& event) {
    auto payload = tao::json::value{
        {"resourceId", event.resource_id().value()},
        {"cancelledBy", event.cancelled_by().value()},
        {"previousStatus", std::string{haven::domain::to_string(event.previous_status())}}};
    if (event.reason().has_value())
        payload["reason"] = *event.reason();
    return payload;
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationExtendedEvent& event) {
    return tao::json::value{
        {"resourceId", event.resource_id().value()},
        {"extendedBy", event.extended_by().value()},
        {"previousStartTime", reservation_timestamp_to_string(event.previous_interval().start())},
        {"previousEndTime", reservation_timestamp_to_string(event.previous_interval().end())},
        {"extendedStartTime", reservation_timestamp_to_string(event.extended_interval().start())},
        {"extendedEndTime", reservation_timestamp_to_string(event.extended_interval().end())}};
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationExpiredEvent& event) {
    return tao::json::value{
        {"resourceId", event.resource_id().value()},
        {"previousStatus", std::string{haven::domain::to_string(event.previous_status())}}};
}

[[nodiscard]] tao::json::value payload_for(const haven::domain::ReservationCompletedEvent& event) {
    auto payload = interval_payload(event.interval());
    payload["resourceId"] = event.resource_id().value();
    return payload;
}

[[nodiscard]] const char* event_type_for(const haven::domain::ReservationCreatedEvent&) {
    return kReservationCreatedEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationConfirmedEvent&) {
    return kReservationConfirmedEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationApprovalRequestedEvent&) {
    return kReservationApprovalRequestedEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationRejectedEvent&) {
    return kReservationRejectedEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationCancelledEvent&) {
    return kReservationCancelledEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationExtendedEvent&) {
    return kReservationExtendedEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationExpiredEvent&) {
    return kReservationExpiredEventType;
}
[[nodiscard]] const char* event_type_for(const haven::domain::ReservationCompletedEvent&) {
    return kReservationCompletedEventType;
}

template <typename Event>
[[nodiscard]] OutboxDocument map_event(const haven::domain::OrganizationId& organization_id,
                                       const haven::domain::ReservationId& reservation_id,
                                       const Event& event) {
    if (event.organization_id() != organization_id || event.reservation_id() != reservation_id) {
        throw std::invalid_argument("Reservation event identity does not match Outbox aggregate");
    }
    auto document = OutboxDocument{.schema_version = kOutboxDocumentSchemaVersion,
                                   .event_id = event.event_id(),
                                   .organization_id = organization_id,
                                   .aggregate_id = reservation_id,
                                   .aggregate_type = kReservationAggregateType,
                                   .event_type = event_type_for(event),
                                   .occurred_at = event.occurred_at(),
                                   .status = OutboxStatus::Pending,
                                   .attempt_count = 0,
                                   .payload = payload_for(event),
                                   .published_at = std::nullopt};
    validate_outbox_document(document);
    return document;
}

}  // namespace

OutboxDocument to_outbox_document(const haven::domain::OrganizationId& organization_id,
                                  const haven::domain::ReservationId& reservation_id,
                                  const haven::domain::ReservationDomainEvent& event) {
    return std::visit(
        [&](const auto& concrete_event) {
            return map_event(organization_id, reservation_id, concrete_event);
        },
        event);
}

}  // namespace haven::infrastructure::persistence::couchbase
