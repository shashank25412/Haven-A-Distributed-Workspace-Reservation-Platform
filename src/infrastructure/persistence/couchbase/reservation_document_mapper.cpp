/**
 * @file reservation_document_mapper.cpp
 * @brief Implements reservation persistence mapping.
 */

#include "haven/infrastructure/persistence/couchbase/reservation_document_mapper.hpp"

#include "haven/domain/value_objects/approval_info.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/rejection_info.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/domain/value_objects/version.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"
#include "haven/logging/logging.hpp"

#include <optional>
#include <string>

namespace haven::infrastructure::persistence::couchbase {
namespace {

[[nodiscard]] std::optional<ReservationApprovalDocument> to_approval_document(
    const std::optional<domain::ApprovalInfo>& approval) {
    if (!approval.has_value()) {
        return std::nullopt;
    }
    return ReservationApprovalDocument{
        .approved_by = approval->approved_by().value(),
        .approved_at = reservation_timestamp_to_string(approval->approved_at()),
    };
}

[[nodiscard]] std::optional<domain::ApprovalInfo> to_domain_approval(
    const std::optional<ReservationApprovalDocument>& approval) {
    if (!approval.has_value()) {
        return std::nullopt;
    }
    return domain::ApprovalInfo{domain::UserId{approval->approved_by},
                                reservation_timestamp_from_string(approval->approved_at)};
}

[[nodiscard]] std::optional<ReservationRejectionDocument> to_rejection_document(
    const std::optional<domain::RejectionInfo>& rejection) {
    if (!rejection.has_value()) {
        return std::nullopt;
    }
    return ReservationRejectionDocument{
        .rejected_by = rejection->rejected_by().value(),
        .rejected_at = reservation_timestamp_to_string(rejection->rejected_at()),
        .reason = rejection->reason(),
    };
}

[[nodiscard]] std::optional<domain::RejectionInfo> to_domain_rejection(
    const std::optional<ReservationRejectionDocument>& rejection) {
    if (!rejection.has_value()) {
        return std::nullopt;
    }
    return domain::RejectionInfo{domain::UserId{rejection->rejected_by},
                                 reservation_timestamp_from_string(rejection->rejected_at),
                                 rejection->reason};
}

}  // namespace

ReservationDocument to_reservation_document(const domain::Reservation& reservation) {
    HVN_TRACE_SCOPE();

    auto document = ReservationDocument{
        .schema_version = kReservationDocumentSchemaVersion,
        .reservation_id = reservation.reservation_id().value(),
        .organization_id = reservation.organization_id().value(),
        .resource_id = reservation.resource_id().value(),
        .created_by = reservation.created_by().value(),
        .start_time = reservation_timestamp_to_string(reservation.interval().start()),
        .end_time = reservation_timestamp_to_string(reservation.interval().end()),
        .purpose = reservation.purpose().value(),
        .status = std::string{domain::to_string(reservation.status())},
        .kind = std::string{domain::to_string(reservation.kind())},
        .approval = to_approval_document(reservation.approval_info()),
        .rejection = to_rejection_document(reservation.rejection_info()),
        .version = reservation.version().value(),
    };
    validate_reservation_document(document);
    return document;
}

domain::Reservation to_domain_reservation(const ReservationDocument& document) {
    HVN_TRACE_SCOPE();
    validate_reservation_document(document);

    return domain::Reservation::rehydrate(
        domain::OrganizationId{document.organization_id},
        domain::ReservationId{document.reservation_id},
        domain::ResourceId{document.resource_id},
        domain::UserId{document.created_by},
        domain::TimeInterval{reservation_timestamp_from_string(document.start_time),
                             reservation_timestamp_from_string(document.end_time)},
        domain::Purpose{document.purpose},
        domain::reservation_kind_from_string(document.kind),
        domain::reservation_status_from_string(document.status),
        to_domain_approval(document.approval),
        to_domain_rejection(document.rejection),
        domain::Version{document.version});
}

}  // namespace haven::infrastructure::persistence::couchbase
