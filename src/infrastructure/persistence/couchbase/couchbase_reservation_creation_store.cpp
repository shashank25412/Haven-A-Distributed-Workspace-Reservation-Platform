/** @file couchbase_reservation_creation_store.cpp */
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_store.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_cas.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document_mapper.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"
#include "haven/infrastructure/persistence/couchbase/reservation_document_mapper.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/transactions.hxx>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace haven::infrastructure::persistence::couchbase {
namespace {
using haven::application::RepositoryError;
using haven::application::RepositoryErrorCode;

struct PendingOutboxInsert final {
    std::string key;
    tao::json::value json;
};

[[nodiscard]] bool has_error(const ::couchbase::error& error, std::error_code expected) {
    auto current = std::optional<::couchbase::error>{error};
    while (current) {
        if (current->ec() == expected)
            return true;
        current = current->cause();
    }
    return false;
}

[[nodiscard]] RepositoryError translate_error(const ::couchbase::error& error) {
    auto code = RepositoryErrorCode::Persistence;
    if (has_error(error, ::couchbase::errc::transaction_op::document_exists) ||
        has_error(error, ::couchbase::errc::key_value::document_exists)) {
        code = RepositoryErrorCode::AlreadyExists;
    } else if (has_error(error,
                         ::couchbase::errc::transaction_op::document_already_in_transaction) ||
               has_error(error, ::couchbase::errc::common::cas_mismatch)) {
        code = RepositoryErrorCode::ConcurrencyConflict;
    } else if (has_error(error, ::couchbase::errc::common::authentication_failure)) {
        code = RepositoryErrorCode::Authentication;
    } else if (has_error(error, ::couchbase::errc::key_value::xattr_no_access)) {
        code = RepositoryErrorCode::Authorization;
    } else if (has_error(error, ::couchbase::errc::common::ambiguous_timeout) ||
               has_error(error, ::couchbase::errc::common::unambiguous_timeout) ||
               has_error(error, ::couchbase::errc::transaction::expired) ||
               has_error(error, ::couchbase::errc::transaction::ambiguous)) {
        code = RepositoryErrorCode::Timeout;
    }
    return RepositoryError{
        code, "Couchbase reservation creation transaction failed: " + error.ec().message()};
}
}  // namespace

CouchbaseReservationCreationStore::CouchbaseReservationCreationStore(
    std::shared_ptr<CouchbaseConnection> connection,
    application::observability::metrics::MetricsRecorder& recorder)
    : connection_(std::move(connection)), metrics_(recorder) {
    if (!connection_)
        throw std::invalid_argument("Couchbase creation store connection is null");
}

haven::application::persistence::PersistenceToken CouchbaseReservationCreationStore::persist(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::Reservation& reservation,
    std::vector<haven::domain::ReservationDomainEvent> domain_events) {
    return metrics_.record_transaction([&] {
        if (reservation.organization_id() != organization_id) {
            throw std::invalid_argument("Cannot create reservation for another organization");
        }
        const auto reservation_key =
            reservation_document_key(organization_id, reservation.reservation_id());
        const auto reservation_json =
            reservation_document_to_json(to_reservation_document(reservation));
        auto outbox_inserts = std::vector<PendingOutboxInsert>{};
        outbox_inserts.reserve(domain_events.size());
        for (const auto& event : domain_events) {
            const auto document =
                to_outbox_document(organization_id, reservation.reservation_id(), event);
            outbox_inserts.push_back({outbox_document_key(organization_id, document.event_id),
                                      outbox_document_to_json(document)});
        }

        auto reservations = connection_->collection(CouchbaseCollections::reservations);
        auto outbox = connection_->collection(CouchbaseCollections::outbox);
        auto [error, result] = connection_->transactions()->run(
            [reservations, outbox, reservation_key, reservation_json, outbox_inserts](
                const std::shared_ptr<::couchbase::transactions::attempt_context>& context) {
                auto [reservation_error, inserted_reservation] =
                    context->insert(reservations, reservation_key, reservation_json);
                static_cast<void>(inserted_reservation);
                if (reservation_error)
                    return reservation_error;
                for (const auto& pending : outbox_inserts) {
                    auto [outbox_error, inserted_outbox] =
                        context->insert(outbox, pending.key, pending.json);
                    static_cast<void>(inserted_outbox);
                    if (outbox_error)
                        return outbox_error;
                }
                return ::couchbase::error{};
            });
        static_cast<void>(result);
        if (error)
            throw translate_error(error);

        // SDK 1.3.2 transaction_result omits committed mutation CAS values.
        auto [read_error, read_result] = reservations.get(reservation_key).get();
        if (read_error) {
            throw RepositoryError{
                RepositoryErrorCode::Persistence,
                "Committed Reservation CAS read failed: " + read_error.ec().message()};
        }
        return persistence_token_from(read_result.cas());
    });
}

}  // namespace haven::infrastructure::persistence::couchbase
