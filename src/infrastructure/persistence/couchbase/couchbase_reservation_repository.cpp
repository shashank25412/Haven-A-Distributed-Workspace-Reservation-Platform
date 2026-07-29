/**
 * @file couchbase_reservation_repository.cpp
 * @brief Implements the Couchbase-backed reservation repository adapter.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"

#include "haven/application/resources/resource_repository_error.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"
#include "haven/logging/logging.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/query_options.hxx>
#include <couchbase/query_scan_consistency.hxx>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using haven::application::resources::ResourceRepositoryError;
using haven::application::resources::ResourceRepositoryErrorCode;
using ReservationListResult = haven::application::reservations::ReservationListResult;

[[nodiscard]] ResourceRepositoryError translate_error(const ::couchbase::error& error,
                                                      const std::string_view operation) {
    const auto error_code = error.ec();
    auto code = ResourceRepositoryErrorCode::Persistence;
    if (error_code == ::couchbase::errc::key_value::document_exists) {
        code = ResourceRepositoryErrorCode::AlreadyExists;
    } else if (error_code == ::couchbase::errc::common::authentication_failure) {
        code = ResourceRepositoryErrorCode::Authentication;
    } else if (error_code == ::couchbase::errc::key_value::xattr_no_access) {
        code = ResourceRepositoryErrorCode::Authorization;
    } else if (error_code == ::couchbase::errc::common::ambiguous_timeout ||
               error_code == ::couchbase::errc::common::unambiguous_timeout) {
        code = ResourceRepositoryErrorCode::Timeout;
    }
    return ResourceRepositoryError{code,
                                   std::string{operation} + " failed: " + error_code.message()};
}

[[nodiscard]] std::string select_prefix() {
    return "SELECT reservation.* FROM `" + std::string{CouchbaseCollections::reservations} +
           "` AS reservation WHERE reservation.documentType = \"reservation\" ";
}

[[nodiscard]] ::couchbase::query_options readonly_options() {
    auto options = ::couchbase::query_options{};
    options.readonly(true).scan_consistency(::couchbase::query_scan_consistency::request_plus);
    return options;
}

[[nodiscard]] ReservationListResult map_rows(const ::couchbase::query_result& result,
                                             const haven::domain::OrganizationId& organization_id,
                                             const std::string_view operation) {
    auto reservations = ReservationListResult{};
    const auto rows = result.rows_as();
    reservations.reserve(rows.size());
    try {
        for (const auto& row : rows) {
            auto reservation = to_domain_reservation(reservation_document_from_json(row));
            if (reservation.organization_id() != organization_id) {
                throw std::invalid_argument("Reservation query crossed organization boundary");
            }
            reservations.push_back(std::move(reservation));
        }
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG(operation,
                      " returned an invalid document for organization ",
                      organization_id.value(),
                      ": ",
                      exception.what());
        throw ResourceRepositoryError{ResourceRepositoryErrorCode::Persistence,
                                      std::string{operation} + " returned an invalid document"};
    }
    return reservations;
}

[[nodiscard]] std::string overlap_predicate() {
    return "AND reservation.resourceId = $resourceId "
           "AND reservation.startTime < $requestedEnd "
           "AND reservation.endTime > $requestedStart ";
}

}  // namespace

CouchbaseReservationRepository::CouchbaseReservationRepository(
    std::shared_ptr<CouchbaseConnection> connection)
    : connection_(std::move(connection)) {
    if (!connection_) {
        throw std::invalid_argument("Couchbase reservation repository connection must not be null");
    }
}

haven::application::reservations::ReservationLookupResult
CouchbaseReservationRepository::find_by_id(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id) const {
    HVN_TRACE_SCOPE();
    const auto key = reservation_document_key(organization_id, reservation_id);
    HVN_DEBUG_LOG("Reading Couchbase reservation document with key ", key);
    auto collection = connection_->collection(CouchbaseCollections::reservations);
    auto [error, result] = collection.get(key).get();
    if (error.ec() == ::couchbase::errc::key_value::document_not_found) {
        return std::nullopt;
    }
    if (error) {
        HVN_ERROR_LOG(
            "Couchbase reservation read failed for organization ",
            organization_id.value(),
            " and reservation ",
            reservation_id.value(),
            ": ",
            error.ec().message());
        throw translate_error(error, "Couchbase reservation read");
    }
    try {
        auto reservation = to_domain_reservation(
            reservation_document_from_json(result.content_as<tao::json::value>()));
        if (reservation.organization_id() != organization_id ||
            reservation.reservation_id() != reservation_id) {
            throw std::invalid_argument("Stored reservation identity does not match its key");
        }
        return reservation;
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG(
            "Stored Couchbase reservation is invalid for organization ",
            organization_id.value(),
            " and reservation ",
            reservation_id.value(),
            ": ",
            exception.what());
        throw ResourceRepositoryError{ResourceRepositoryErrorCode::Persistence,
                                      "Stored Couchbase reservation document is invalid"};
    }
}

ReservationListResult CouchbaseReservationRepository::find_by_creator(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::UserId& caller_id) const {
    HVN_TRACE_SCOPE();
    auto options = readonly_options();
    options.named_parameters(std::make_pair("organizationId", organization_id.value()),
                             std::make_pair("createdBy", caller_id.value()));
    const auto statement = select_prefix() +
                           "AND reservation.organizationId = $organizationId "
                           "AND reservation.createdBy = $createdBy";
    auto [error, result] = connection_->scope().query(statement, options).get();
    if (error) {
        HVN_ERROR_LOG("Couchbase reservation creator query failed for organization ",
                      organization_id.value(),
                      ": ",
                      error.ec().message());
        throw translate_error(error, "Couchbase reservation creator query");
    }
    return map_rows(result, organization_id, "Couchbase reservation creator query");
}

ReservationListResult CouchbaseReservationRepository::find_pending_approvals(
    const haven::domain::OrganizationId& organization_id) const {
    HVN_TRACE_SCOPE();
    auto options = readonly_options();
    options.named_parameters(
        std::make_pair("organizationId", organization_id.value()),
        std::make_pair("status",
                       std::string{haven::domain::to_string(
                           haven::domain::ReservationStatus::PendingApproval)}));
    const auto statement = select_prefix() +
                           "AND reservation.organizationId = $organizationId "
                           "AND reservation.status = $status";
    auto [error, result] = connection_->scope().query(statement, options).get();
    if (error) {
        HVN_ERROR_LOG("Couchbase pending approval query failed for organization ",
                      organization_id.value(),
                      ": ",
                      error.ec().message());
        throw translate_error(error, "Couchbase pending approval query");
    }
    return map_rows(result, organization_id, "Couchbase pending approval query");
}

ReservationListResult CouchbaseReservationRepository::find_by_resource_and_interval(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::TimeInterval& interval) const {
    HVN_TRACE_SCOPE();
    auto options = readonly_options();
    options.named_parameters(
        std::make_pair("organizationId", organization_id.value()),
        std::make_pair("resourceId", resource_id.value()),
        std::make_pair("requestedStart", reservation_timestamp_to_string(interval.start())),
        std::make_pair("requestedEnd", reservation_timestamp_to_string(interval.end())));
    const auto statement =
        select_prefix() + "AND reservation.organizationId = $organizationId " + overlap_predicate();
    auto [error, result] = connection_->scope().query(statement, options).get();
    if (error) {
        HVN_ERROR_LOG(
            "Couchbase reservation calendar query failed for organization ",
            organization_id.value(),
            " and resource ",
            resource_id.value(),
            ": ",
            error.ec().message());
        throw translate_error(error, "Couchbase reservation calendar query");
    }
    auto reservations = map_rows(result, organization_id, "Couchbase reservation calendar query");
    for (const auto& reservation : reservations) {
        if (reservation.resource_id() != resource_id ||
            !reservation.interval().overlaps(interval)) {
            throw ResourceRepositoryError{
                ResourceRepositoryErrorCode::Persistence,
                "Couchbase reservation calendar query returned a document outside its filter"};
        }
    }
    return reservations;
}

bool CouchbaseReservationRepository::has_conflict(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::TimeInterval& interval) const {
    HVN_TRACE_SCOPE();
    auto options = readonly_options();
    options.named_parameters(
        std::make_pair("organizationId", organization_id.value()),
        std::make_pair("resourceId", resource_id.value()),
        std::make_pair("requestedStart", reservation_timestamp_to_string(interval.start())),
        std::make_pair("requestedEnd", reservation_timestamp_to_string(interval.end())),
        std::make_pair(
            "status",
            std::string{haven::domain::to_string(haven::domain::ReservationStatus::Confirmed)}));
    const auto statement = select_prefix() + "AND reservation.organizationId = $organizationId " +
                           overlap_predicate() + "AND reservation.status = $status LIMIT 1";
    auto [error, result] = connection_->scope().query(statement, options).get();
    if (error) {
        HVN_ERROR_LOG(
            "Couchbase reservation conflict query failed for organization ",
            organization_id.value(),
            " and resource ",
            resource_id.value(),
            ": ",
            error.ec().message());
        throw translate_error(error, "Couchbase reservation conflict query");
    }
    return !result.rows_as().empty();
}

bool CouchbaseReservationRepository::has_conflict_excluding(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::TimeInterval& interval,
    const haven::domain::ReservationId& excluded_reservation_id) const {
    HVN_TRACE_SCOPE();
    auto options = readonly_options();
    options.named_parameters(
        std::make_pair("organizationId", organization_id.value()),
        std::make_pair("resourceId", resource_id.value()),
        std::make_pair("requestedStart", reservation_timestamp_to_string(interval.start())),
        std::make_pair("requestedEnd", reservation_timestamp_to_string(interval.end())),
        std::make_pair(
            "status",
            std::string{haven::domain::to_string(haven::domain::ReservationStatus::Confirmed)}),
        std::make_pair("excludedReservationId", excluded_reservation_id.value()));
    const auto statement = select_prefix() + "AND reservation.organizationId = $organizationId " +
                           overlap_predicate() +
                           "AND reservation.status = $status "
                           "AND reservation.reservationId != $excludedReservationId LIMIT 1";
    auto [error, result] = connection_->scope().query(statement, options).get();
    if (error) {
        HVN_ERROR_LOG(
            "Couchbase excluding conflict query failed for organization ",
            organization_id.value(),
            " and resource ",
            resource_id.value(),
            ": ",
            error.ec().message());
        throw translate_error(error, "Couchbase excluding conflict query");
    }
    return !result.rows_as().empty();
}

void CouchbaseReservationRepository::save(const haven::domain::OrganizationId& organization_id,
                                          const haven::domain::Reservation& reservation) {
    HVN_TRACE_SCOPE();
    if (reservation.organization_id() != organization_id) {
        throw std::invalid_argument("Cannot save a reservation under a different organization");
    }
    const auto key = reservation_document_key(organization_id, reservation.reservation_id());
    const auto json = reservation_document_to_json(to_reservation_document(reservation));
    HVN_DEBUG_LOG("Upserting Couchbase reservation document with key ", key);
    auto collection = connection_->collection(CouchbaseCollections::reservations);
    auto [error, result] = collection.upsert(key, json).get();
    static_cast<void>(result);
    if (error) {
        HVN_ERROR_LOG(
            "Couchbase reservation save failed for organization ",
            organization_id.value(),
            " and reservation ",
            reservation.reservation_id().value(),
            ": ",
            error.ec().message());
        throw translate_error(error, "Couchbase reservation save");
    }
}

}  // namespace haven::infrastructure::persistence::couchbase
