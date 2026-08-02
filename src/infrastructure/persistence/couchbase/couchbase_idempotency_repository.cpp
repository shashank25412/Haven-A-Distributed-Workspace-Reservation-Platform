#include "haven/infrastructure/persistence/couchbase/couchbase_idempotency_repository.hpp"

#include "haven/application/idempotency/idempotency_repository_error.hpp"
#include "haven/application/repository_error.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/idempotency_document_mapper.hpp"
#include "haven/logging/logging.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/get_result.hxx>
#include <couchbase/insert_options.hxx>
#include <couchbase/replace_options.hxx>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using namespace haven::application;
using namespace haven::application::idempotency;

RepositoryError translate_error(const ::couchbase::error& error, const std::string_view operation) {
    auto code = RepositoryErrorCode::Persistence;
    if (error.ec() == ::couchbase::errc::common::authentication_failure)
        code = RepositoryErrorCode::Authentication;
    else if (error.ec() == ::couchbase::errc::key_value::xattr_no_access)
        code = RepositoryErrorCode::Authorization;
    else if (error.ec() == ::couchbase::errc::common::ambiguous_timeout ||
             error.ec() == ::couchbase::errc::common::unambiguous_timeout)
        code = RepositoryErrorCode::Timeout;
    else if (error.ec() == ::couchbase::errc::common::cas_mismatch)
        code = RepositoryErrorCode::ConcurrencyConflict;
    return RepositoryError{code, std::string{operation} + " failed: " + error.ec().message()};
}

IdempotencyRecord map_result(const ::couchbase::get_result& result) {
    try {
        return to_idempotency_record(
            idempotency_document_from_json(result.content_as<tao::json::value>()));
    } catch (const std::exception& exception) {
        HVN_ERROR_LOG("Stored Couchbase idempotency document is invalid: ", exception.what());
        throw RepositoryError{RepositoryErrorCode::Persistence,
                              "Stored Couchbase idempotency document is invalid"};
    }
}

IdempotencyClaimResult classify(const IdempotencyRecord& existing,
                                const IdempotencyFingerprint& fingerprint) {
    if (existing.fingerprint() != fingerprint)
        return IdempotencyClaimResult::fingerprint_mismatch(existing);
    switch (existing.status()) {
        case IdempotencyStatus::Processing:
            return IdempotencyClaimResult::existing_processing(existing);
        case IdempotencyStatus::Succeeded:
            return IdempotencyClaimResult::existing_succeeded(existing);
        case IdempotencyStatus::FailedPermanent:
            return IdempotencyClaimResult::existing_failed_permanently(existing);
    }
    throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::InvalidRecord,
                                     "Stored idempotency record has an invalid state"};
}

void validate_completion(const IdempotencyRecord& record,
                         const IdempotencyFingerprint& fingerprint,
                         const CreateReservationResultSnapshot& snapshot,
                         const bool succeeded) {
    if (record.fingerprint() != fingerprint)
        throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::FingerprintMismatch,
                                         "Idempotency fingerprint does not match"};
    if (snapshot.is_success() != succeeded)
        throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::InvalidSnapshot,
                                         "Idempotency completion snapshot has the wrong outcome"};
    if (succeeded && (*snapshot.reservation_id() != record.generated_identifiers().reservation_id ||
                      *snapshot.created_at() != record.created_at()))
        throw IdempotencyRepositoryError{
            IdempotencyRepositoryErrorCode::InvalidSnapshot,
            "Idempotency completion snapshot does not match the claim"};
    if (record.status() != IdempotencyStatus::Processing) {
        const bool equivalent =
            record.result().has_value() && *record.result() == snapshot &&
            ((succeeded && record.status() == IdempotencyStatus::Succeeded) ||
             (!succeeded && record.status() == IdempotencyStatus::FailedPermanent));
        if (equivalent)
            return;
        throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::TerminalConflict,
                                         "Idempotency record already has a different outcome"};
    }
}

}  // namespace

CouchbaseIdempotencyRepository::CouchbaseIdempotencyRepository(
    std::shared_ptr<CouchbaseConnection> connection, const std::chrono::seconds retention)
    : connection_(std::move(connection)), retention_(retention) {
    if (!connection_)
        throw std::invalid_argument("Idempotency repository connection is null");
    if (retention_ <= std::chrono::seconds::zero())
        throw std::invalid_argument("Idempotency retention must be positive");
}

IdempotencyClaimResult CouchbaseIdempotencyRepository::claim(
    const IdempotencyRecord& processing_record) {
    HVN_TRACE_SCOPE();
    if (processing_record.status() != IdempotencyStatus::Processing)
        throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::InvalidRecord,
                                         "Only a processing idempotency record may be claimed"};
    const auto key = idempotency_document_key(processing_record.scope());
    auto options = ::couchbase::insert_options{};
    options.expiry(retention_);
    auto collection = connection_->collection(CouchbaseCollections::idempotency);
    auto [error, result] =
        collection
            .insert(key,
                    idempotency_document_to_json(to_idempotency_document(processing_record)),
                    options)
            .get();
    static_cast<void>(result);
    if (!error)
        return IdempotencyClaimResult::claimed(processing_record);
    if (error.ec() != ::couchbase::errc::key_value::document_exists &&
        error.ec() != ::couchbase::errc::common::ambiguous_timeout)
        throw translate_error(error, "Couchbase idempotency claim");
    auto [read_error, read_result] = collection.get(key).get();
    if (read_error) {
        if (error.ec() == ::couchbase::errc::common::ambiguous_timeout &&
            read_error.ec() == ::couchbase::errc::key_value::document_not_found)
            throw translate_error(error, "Couchbase idempotency claim");
        throw translate_error(read_error, "Couchbase idempotency claim reconciliation");
    }
    return classify(map_result(read_result), processing_record.fingerprint());
}

std::optional<IdempotencyRecord> CouchbaseIdempotencyRepository::find(
    const IdempotencyScope& scope) const {
    HVN_TRACE_SCOPE();
    auto collection = connection_->collection(CouchbaseCollections::idempotency);
    auto [error, result] = collection.get(idempotency_document_key(scope)).get();
    if (error.ec() == ::couchbase::errc::key_value::document_not_found)
        return std::nullopt;
    if (error)
        throw translate_error(error, "Couchbase idempotency read");
    auto record = map_result(result);
    if (record.scope() != scope)
        throw RepositoryError{RepositoryErrorCode::Persistence,
                              "Stored idempotency scope does not match its key"};
    return record;
}

void CouchbaseIdempotencyRepository::record_succeeded(
    const IdempotencyScope& scope,
    const IdempotencyFingerprint& fingerprint,
    const CreateReservationResultSnapshot& snapshot) {
    complete(scope, fingerprint, snapshot, true);
}

void CouchbaseIdempotencyRepository::record_failed_permanently(
    const IdempotencyScope& scope,
    const IdempotencyFingerprint& fingerprint,
    const CreateReservationResultSnapshot& snapshot) {
    complete(scope, fingerprint, snapshot, false);
}

void CouchbaseIdempotencyRepository::complete(const IdempotencyScope& scope,
                                              const IdempotencyFingerprint& fingerprint,
                                              const CreateReservationResultSnapshot& snapshot,
                                              const bool succeeded) {
    HVN_TRACE_SCOPE();
    const auto key = idempotency_document_key(scope);
    auto collection = connection_->collection(CouchbaseCollections::idempotency);
    auto [read_error, read_result] = collection.get(key).get();
    if (read_error.ec() == ::couchbase::errc::key_value::document_not_found)
        throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::MissingRecord,
                                         "Idempotency record does not exist"};
    if (read_error)
        throw translate_error(read_error, "Couchbase idempotency completion read");
    auto record = map_result(read_result);
    if (record.scope() != scope)
        throw RepositoryError{RepositoryErrorCode::Persistence,
                              "Stored idempotency scope does not match its key"};
    validate_completion(record, fingerprint, snapshot, succeeded);
    if (record.status() != IdempotencyStatus::Processing)
        return;
    if (succeeded)
        record.record_succeeded(snapshot);
    else
        record.record_failed_permanently(snapshot);
    auto options = ::couchbase::replace_options{};
    options.cas(read_result.cas()).preserve_expiry(true);
    auto [replace_error, replace_result] =
        collection
            .replace(key, idempotency_document_to_json(to_idempotency_document(record)), options)
            .get();
    static_cast<void>(replace_result);
    if (!replace_error)
        return;
    if (replace_error.ec() != ::couchbase::errc::common::cas_mismatch)
        throw translate_error(replace_error, "Couchbase idempotency completion");
    HVN_WARN_LOG("Reconciling concurrent Couchbase idempotency completion");
    auto [reconcile_error, reconcile_result] = collection.get(key).get();
    if (reconcile_error)
        throw translate_error(reconcile_error, "Couchbase idempotency reconciliation");
    auto reconciled = map_result(reconcile_result);
    if (reconciled.scope() != scope)
        throw RepositoryError{RepositoryErrorCode::Persistence,
                              "Stored idempotency scope does not match its key"};
    validate_completion(reconciled, fingerprint, snapshot, succeeded);
    if (reconciled.status() == IdempotencyStatus::Processing)
        throw RepositoryError{RepositoryErrorCode::ConcurrencyConflict,
                              "Idempotency record changed concurrently"};
}

}  // namespace haven::infrastructure::persistence::couchbase
