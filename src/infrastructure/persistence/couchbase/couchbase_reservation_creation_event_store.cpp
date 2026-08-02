/**
 * @file couchbase_reservation_creation_event_store.cpp
 * @brief Implements Couchbase creation-event recovery reads.
 */
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_event_store.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {
namespace {
[[nodiscard]] haven::application::RepositoryError translate(const ::couchbase::error& error) {
    auto code = haven::application::RepositoryErrorCode::Persistence;
    if (error.ec() == ::couchbase::errc::common::authentication_failure)
        code = haven::application::RepositoryErrorCode::Authentication;
    else if (error.ec() == ::couchbase::errc::key_value::xattr_no_access)
        code = haven::application::RepositoryErrorCode::Authorization;
    else if (error.ec() == ::couchbase::errc::common::ambiguous_timeout ||
             error.ec() == ::couchbase::errc::common::unambiguous_timeout)
        code = haven::application::RepositoryErrorCode::Timeout;
    return {code, "Couchbase creation-event read failed: " + error.ec().message()};
}
}  // namespace

CouchbaseReservationCreationEventStore::CouchbaseReservationCreationEventStore(
    std::shared_ptr<CouchbaseConnection> connection)
    : connection_(std::move(connection)) {
    if (!connection_)
        throw std::invalid_argument("Creation-event store connection is null");
}

bool CouchbaseReservationCreationEventStore::contains_all(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id,
    const std::vector<haven::domain::EventId>& event_ids) const {
    if (event_ids.size() != 2)
        return false;
    auto collection = connection_->collection(CouchbaseCollections::outbox);
    for (std::size_t index = 0; index < event_ids.size(); ++index) {
        const auto& event_id = event_ids[index];
        auto [error, result] = collection.get(outbox_document_key(organization_id, event_id)).get();
        if (error.ec() == ::couchbase::errc::key_value::document_not_found)
            return false;
        if (error)
            throw translate(error);
        try {
            const auto document = outbox_document_from_json(result.content_as<tao::json::value>());
            if (document.organization_id != organization_id ||
                document.aggregate_id != reservation_id || document.event_id != event_id)
                throw std::invalid_argument("Outbox creation-event identity mismatch");
            const auto creation_compatible =
                index == 0 ? document.event_type == kReservationCreatedEventType
                           : (document.event_type == kReservationConfirmedEventType ||
                              document.event_type == kReservationApprovalRequestedEventType);
            if (!creation_compatible)
                throw std::invalid_argument("Outbox event is not creation-compatible");
        } catch (const std::exception& exception) {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence,
                std::string{"Invalid persisted creation event: "} + exception.what()};
        }
    }
    return true;
}

}  // namespace haven::infrastructure::persistence::couchbase
