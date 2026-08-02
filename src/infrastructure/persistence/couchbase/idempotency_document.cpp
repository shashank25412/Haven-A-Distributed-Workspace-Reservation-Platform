/**
 * @file idempotency_document.cpp
 * @brief Implements the persisted Couchbase idempotency document model.
 */

#include "haven/infrastructure/persistence/couchbase/idempotency_document.hpp"

#include "haven/infrastructure/persistence/couchbase/idempotency_document_validator.hpp"

#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {
namespace {

template <typename T>
void add_optional(tao::json::value& json, const char* name, const std::optional<T>& value) {
    if (value.has_value()) {
        json[name] = *value;
    }
}

std::optional<IdempotencyResultDocument> result_from_json(const tao::json::value& json) {
    const auto& object = json.get_object();
    const auto found = object.find("result");
    if (found == object.end()) {
        return std::nullopt;
    }
    const auto& result = found->second;
    const auto optional_string = [&result](const char* name) -> std::optional<std::string> {
        const auto& object = result.get_object();
        const auto value = object.find(name);
        return value == object.end() ? std::nullopt
                                     : std::optional<std::string>{value->second.get_string()};
    };
    const auto optional_unsigned = [&result](const char* name) -> std::optional<std::uint64_t> {
        const auto& object = result.get_object();
        const auto value = object.find(name);
        return value == object.end() ? std::nullopt
                                     : std::optional<std::uint64_t>{value->second.get_unsigned()};
    };
    return IdempotencyResultDocument{
        .creation_status = result.at("creationStatus").get_string(),
        .organization_id = optional_string("organizationId"),
        .reservation_id = optional_string("reservationId"),
        .resource_id = optional_string("resourceId"),
        .creator_id = optional_string("creatorId"),
        .interval_start = optional_string("intervalStart"),
        .interval_end = optional_string("intervalEnd"),
        .purpose = optional_string("purpose"),
        .reservation_status = optional_string("reservationStatus"),
        .reservation_kind = optional_string("reservationKind"),
        .initial_version = optional_unsigned("initialVersion"),
        .created_at = optional_string("createdAt"),
    };
}

}  // namespace

tao::json::value idempotency_document_to_json(const IdempotencyDocument& document) {
    validate_idempotency_document(document);
    tao::json::value json{{"documentType", kIdempotencyDocumentType},
                          {"schemaVersion", document.schema_version},
                          {"organizationId", document.organization_id},
                          {"creatorId", document.creator_id},
                          {"operation", document.operation},
                          {"idempotencyKey", document.idempotency_key},
                          {"fingerprint", document.fingerprint},
                          {"status", document.status},
                          {"reservationId", document.reservation_id},
                          {"createdEventId", document.created_event_id},
                          {"confirmedEventId", document.confirmed_event_id},
                          {"approvalRequestedEventId", document.approval_requested_event_id},
                          {"createdAt", document.created_at}};
    if (document.result.has_value()) {
        tao::json::value result{{"creationStatus", document.result->creation_status}};
        add_optional(result, "organizationId", document.result->organization_id);
        add_optional(result, "reservationId", document.result->reservation_id);
        add_optional(result, "resourceId", document.result->resource_id);
        add_optional(result, "creatorId", document.result->creator_id);
        add_optional(result, "intervalStart", document.result->interval_start);
        add_optional(result, "intervalEnd", document.result->interval_end);
        add_optional(result, "purpose", document.result->purpose);
        add_optional(result, "reservationStatus", document.result->reservation_status);
        add_optional(result, "reservationKind", document.result->reservation_kind);
        add_optional(result, "initialVersion", document.result->initial_version);
        add_optional(result, "createdAt", document.result->created_at);
        json["result"] = std::move(result);
    }
    return json;
}

IdempotencyDocument idempotency_document_from_json(const tao::json::value& json) {
    if (json.at("documentType").get_string() != kIdempotencyDocumentType) {
        throw std::invalid_argument("Unexpected Couchbase idempotency document type");
    }
    auto document = IdempotencyDocument{
        .schema_version = json.at("schemaVersion").get_unsigned(),
        .organization_id = json.at("organizationId").get_string(),
        .creator_id = json.at("creatorId").get_string(),
        .operation = json.at("operation").get_string(),
        .idempotency_key = json.at("idempotencyKey").get_string(),
        .fingerprint = json.at("fingerprint").get_string(),
        .status = json.at("status").get_string(),
        .reservation_id = json.at("reservationId").get_string(),
        .created_event_id = json.at("createdEventId").get_string(),
        .confirmed_event_id = json.at("confirmedEventId").get_string(),
        .approval_requested_event_id = json.at("approvalRequestedEventId").get_string(),
        .created_at = json.at("createdAt").get_string(),
        .result = result_from_json(json),
    };
    validate_idempotency_document(document);
    return document;
}

}  // namespace haven::infrastructure::persistence::couchbase
