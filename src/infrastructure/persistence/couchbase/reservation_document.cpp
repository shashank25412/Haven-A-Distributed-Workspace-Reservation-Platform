/**
 * @file reservation_document.cpp
 * @brief Implements reservation document JSON conversion.
 */

#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"

#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"
#include "haven/logging/logging.hpp"

#include <stdexcept>
#include <string>

namespace haven::infrastructure::persistence::couchbase {
namespace {

void validate_document_type(const tao::json::value& json) {
    const auto& document_type = json.at("documentType").get_string();
    if (document_type != kReservationDocumentType) {
        throw std::invalid_argument("Unexpected Couchbase document type for reservation: " +
                                    document_type);
    }
}

[[nodiscard]] std::optional<ReservationApprovalDocument> approval_from_json(
    const tao::json::value& json) {
    const auto& object = json.get_object();
    const auto approval = object.find("approval");
    if (approval == object.end()) {
        return std::nullopt;
    }

    return ReservationApprovalDocument{
        .approved_by = approval->second.at("approvedBy").get_string(),
        .approved_at = approval->second.at("approvedAt").get_string(),
    };
}

[[nodiscard]] std::optional<ReservationRejectionDocument> rejection_from_json(
    const tao::json::value& json) {
    const auto& object = json.get_object();
    const auto rejection = object.find("rejection");
    if (rejection == object.end()) {
        return std::nullopt;
    }

    const auto& rejection_object = rejection->second.get_object();
    const auto reason = rejection_object.find("reason");

    return ReservationRejectionDocument{
        .rejected_by = rejection->second.at("rejectedBy").get_string(),
        .rejected_at = rejection->second.at("rejectedAt").get_string(),
        .reason = reason == rejection_object.end()
                      ? std::nullopt
                      : std::optional<std::string>{reason->second.get_string()},
    };
}

[[nodiscard]] std::optional<ReservationCancellationDocument> cancellation_from_json(
    const tao::json::value& json) {
    const auto& object = json.get_object();
    const auto cancellation = object.find("cancellation");
    if (cancellation == object.end()) {
        return std::nullopt;
    }

    const auto& cancellation_object = cancellation->second.get_object();
    const auto reason = cancellation_object.find("reason");

    return ReservationCancellationDocument{
        .cancelled_by = cancellation->second.at("cancelledBy").get_string(),
        .cancelled_at = cancellation->second.at("cancelledAt").get_string(),
        .reason = reason == cancellation_object.end()
                      ? std::nullopt
                      : std::optional<std::string>{reason->second.get_string()},
    };
}

}  // namespace

tao::json::value reservation_document_to_json(const ReservationDocument& document) {
    HVN_TRACE_SCOPE();
    validate_reservation_document(document);

    tao::json::value json{
        {"documentType", kReservationDocumentType},
        {"schemaVersion", document.schema_version},
        {"reservationId", document.reservation_id},
        {"organizationId", document.organization_id},
        {"resourceId", document.resource_id},
        {"createdBy", document.created_by},
        {"startTime", document.start_time},
        {"endTime", document.end_time},
        {"purpose", document.purpose},
        {"status", document.status},
        {"kind", document.kind},
        {"version", document.version},
    };
    if (document.approval.has_value()) {
        json["approval"] = tao::json::value{
            {"approvedBy", document.approval->approved_by},
            {"approvedAt", document.approval->approved_at},
        };
    }
    if (document.rejection.has_value()) {
        tao::json::value rejection_json{
            {"rejectedBy", document.rejection->rejected_by},
            {"rejectedAt", document.rejection->rejected_at},
        };
        if (document.rejection->reason.has_value()) {
            rejection_json["reason"] = *document.rejection->reason;
        }
        json["rejection"] = std::move(rejection_json);
    }
    if (document.cancellation.has_value()) {
        tao::json::value cancellation_json{
            {"cancelledBy", document.cancellation->cancelled_by},
            {"cancelledAt", document.cancellation->cancelled_at},
        };
        if (document.cancellation->reason.has_value()) {
            cancellation_json["reason"] = *document.cancellation->reason;
        }
        json["cancellation"] = std::move(cancellation_json);
    }
    return json;
}

ReservationDocument reservation_document_from_json(const tao::json::value& json) {
    HVN_TRACE_SCOPE();
    validate_document_type(json);

    auto document = ReservationDocument{
        .schema_version = json.at("schemaVersion").get_unsigned(),
        .reservation_id = json.at("reservationId").get_string(),
        .organization_id = json.at("organizationId").get_string(),
        .resource_id = json.at("resourceId").get_string(),
        .created_by = json.at("createdBy").get_string(),
        .start_time = json.at("startTime").get_string(),
        .end_time = json.at("endTime").get_string(),
        .purpose = json.at("purpose").get_string(),
        .status = json.at("status").get_string(),
        .kind = json.at("kind").get_string(),
        .approval = approval_from_json(json),
        .rejection = rejection_from_json(json),
        .cancellation = cancellation_from_json(json),
        .version = json.at("version").get_unsigned(),
    };
    validate_reservation_document(document);
    return document;
}

}  // namespace haven::infrastructure::persistence::couchbase
