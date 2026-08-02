/**
 * @file outbox_document.cpp
 * @brief Implements Outbox document JSON conversion.
 */

#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"

#include "haven/infrastructure/persistence/couchbase/outbox_document_validator.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include <limits>
#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] std::optional<OutboxDocument::TimePoint> published_at_from_json(
    const tao::json::value& json) {
    const auto found = json.get_object().find("publishedAt");
    if (found == json.get_object().end())
        return std::nullopt;
    return reservation_timestamp_from_string(found->second.get_string());
}

}  // namespace

tao::json::value outbox_document_to_json(const OutboxDocument& document) {
    validate_outbox_document(document);
    tao::json::value json{{"documentType", kOutboxDocumentType},
                          {"schemaVersion", document.schema_version},
                          {"eventId", document.event_id.value()},
                          {"organizationId", document.organization_id.value()},
                          {"aggregateId", document.aggregate_id.value()},
                          {"aggregateType", document.aggregate_type},
                          {"eventType", document.event_type},
                          {"occurredAt", reservation_timestamp_to_string(document.occurred_at)},
                          {"status", std::string{to_string(document.status)}},
                          {"attemptCount", document.attempt_count},
                          {"payload", document.payload}};
    if (document.published_at.has_value()) {
        json["publishedAt"] = reservation_timestamp_to_string(*document.published_at);
    }
    return json;
}

OutboxDocument outbox_document_from_json(const tao::json::value& json) {
    try {
        if (json.at("documentType").get_string() != kOutboxDocumentType) {
            throw std::invalid_argument("Unexpected Couchbase Outbox document type");
        }
        const auto attempt_count = json.at("attemptCount").get_unsigned();
        if (attempt_count > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument("Outbox attempt count is out of range");
        }
        auto document = OutboxDocument{
            .schema_version = json.at("schemaVersion").get_unsigned(),
            .event_id = haven::domain::EventId{json.at("eventId").get_string()},
            .organization_id =
                haven::domain::OrganizationId{json.at("organizationId").get_string()},
            .aggregate_id = haven::domain::ReservationId{json.at("aggregateId").get_string()},
            .aggregate_type = json.at("aggregateType").get_string(),
            .event_type = json.at("eventType").get_string(),
            .occurred_at = reservation_timestamp_from_string(json.at("occurredAt").get_string()),
            .status = outbox_status_from_string(json.at("status").get_string()),
            .attempt_count = static_cast<std::uint32_t>(attempt_count),
            .payload = json.at("payload"),
            .published_at = published_at_from_json(json),
        };
        validate_outbox_document(document);
        return document;
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::exception& exception) {
        throw std::invalid_argument(std::string{"Invalid Couchbase Outbox document: "} +
                                    exception.what());
    }
}

}  // namespace haven::infrastructure::persistence::couchbase
