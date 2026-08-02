/** @file couchbase_outbox_repository.cpp */
#include "haven/infrastructure/persistence/couchbase/couchbase_outbox_repository.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_cas.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_collections.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_document.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_message_mapper.hpp"

#include <couchbase/codec/tao_json_serializer.hxx>
#include <couchbase/error_codes.hxx>
#include <couchbase/query_options.hxx>
#include <couchbase/query_scan_consistency.hxx>
#include <couchbase/replace_options.hxx>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {
namespace {
using haven::application::RepositoryError;
using haven::application::RepositoryErrorCode;
using haven::application::outbox::LoadedOutboxMessage;

[[nodiscard]] RepositoryError error_for(const ::couchbase::error& error,
                                        const std::string& operation) {
    auto code = RepositoryErrorCode::Persistence;
    if (error.ec() == ::couchbase::errc::common::cas_mismatch)
        code = RepositoryErrorCode::ConcurrencyConflict;
    else if (error.ec() == ::couchbase::errc::common::authentication_failure)
        code = RepositoryErrorCode::Authentication;
    else if (error.ec() == ::couchbase::errc::key_value::xattr_no_access)
        code = RepositoryErrorCode::Authorization;
    else if (error.ec() == ::couchbase::errc::common::ambiguous_timeout ||
             error.ec() == ::couchbase::errc::common::unambiguous_timeout)
        code = RepositoryErrorCode::Timeout;
    return {code, operation + " failed: " + error.ec().message()};
}

[[nodiscard]] LoadedOutboxMessage loaded(const OutboxDocument& document, ::couchbase::cas cas) {
    return {to_outbox_message(document), persistence_token_from(cas)};
}

[[nodiscard]] OutboxDocument read_document(const ::couchbase::get_result& result) {
    try {
        return outbox_document_from_json(result.content_as<tao::json::value>());
    } catch (const std::exception& exception) {
        throw RepositoryError{
            RepositoryErrorCode::Persistence,
            std::string{"Stored Outbox document is invalid: "} + exception.what()};
    }
}
}  // namespace

CouchbaseOutboxRepository::CouchbaseOutboxRepository(
    std::shared_ptr<CouchbaseConnection> connection)
    : connection_(std::move(connection)) {
    if (!connection_)
        throw std::invalid_argument("Couchbase Outbox repository connection is null");
}

std::vector<LoadedOutboxMessage> CouchbaseOutboxRepository::find_pending(std::size_t limit) const {
    if (limit == 0)
        throw std::invalid_argument("Outbox pending limit must be positive");
    auto options = ::couchbase::query_options{};
    options.readonly(true)
        .scan_consistency(::couchbase::query_scan_consistency::request_plus)
        .named_parameters(std::make_pair("limit", static_cast<std::uint64_t>(limit)));
    const auto statement = "SELECT outbox AS document, META(outbox).cas AS cas FROM `" +
                           std::string{CouchbaseCollections::outbox} +
                           "` AS outbox WHERE outbox.documentType = \"outbox\" "
                           "AND outbox.status = \"PENDING\" "
                           "ORDER BY outbox.occurredAt, outbox.eventId LIMIT $limit";
    auto [error, result] = connection_->scope().query(statement, options).get();
    if (error)
        throw error_for(error, "Couchbase pending Outbox query");
    auto messages = std::vector<LoadedOutboxMessage>{};
    try {
        for (const auto& row : result.rows_as()) {
            const auto document = outbox_document_from_json(row.at("document"));
            messages.emplace_back(
                to_outbox_message(document),
                haven::application::persistence::PersistenceToken{row.at("cas").get_unsigned()});
        }
    } catch (const std::exception& exception) {
        throw RepositoryError{
            RepositoryErrorCode::Persistence,
            std::string{"Pending Outbox query returned invalid data: "} + exception.what()};
    }
    return messages;
}

std::optional<LoadedOutboxMessage> CouchbaseOutboxRepository::claim(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::EventId& event_id,
    const haven::application::persistence::PersistenceToken& expected_token) {
    auto collection = connection_->collection(CouchbaseCollections::outbox);
    auto [get_error, result] = collection.get(outbox_document_key(organization_id, event_id)).get();
    if (get_error.ec() == ::couchbase::errc::key_value::document_not_found)
        return std::nullopt;
    if (get_error)
        throw error_for(get_error, "Couchbase Outbox claim read");
    if (result.cas() != couchbase_cas_from(expected_token))
        return std::nullopt;
    auto document = read_document(result);
    if (document.organization_id != organization_id || document.event_id != event_id)
        throw RepositoryError{RepositoryErrorCode::Persistence, "Stored Outbox identity mismatch"};
    if (document.status != OutboxStatus::Pending)
        return std::nullopt;
    document.status = OutboxStatus::Publishing;
    ++document.attempt_count;
    auto options = ::couchbase::replace_options{};
    options.cas(couchbase_cas_from(expected_token));
    auto [replace_error, replaced] = collection
                                         .replace(outbox_document_key(organization_id, event_id),
                                                  outbox_document_to_json(document),
                                                  options)
                                         .get();
    if (replace_error.ec() == ::couchbase::errc::common::cas_mismatch)
        return std::nullopt;
    if (replace_error)
        throw error_for(replace_error, "Couchbase Outbox claim");
    return loaded(document, replaced.cas());
}

namespace {
template <typename Mutation>
LoadedOutboxMessage mutate_publishing(
    CouchbaseConnection& connection,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::EventId& event_id,
    const haven::application::persistence::PersistenceToken& token,
    Mutation mutation,
    const std::string& operation) {
    auto collection = connection.collection(CouchbaseCollections::outbox);
    const auto key = outbox_document_key(organization_id, event_id);
    auto [get_error, result] = collection.get(key).get();
    if (get_error)
        throw error_for(get_error, operation + " read");
    if (result.cas() != couchbase_cas_from(token))
        throw RepositoryError{RepositoryErrorCode::ConcurrencyConflict,
                              operation + " has stale token"};
    auto document = read_document(result);
    if (document.organization_id != organization_id || document.event_id != event_id)
        throw RepositoryError{RepositoryErrorCode::Persistence, "Stored Outbox identity mismatch"};
    if (document.status != OutboxStatus::Publishing)
        throw RepositoryError{RepositoryErrorCode::ConcurrencyConflict,
                              operation + " requires PUBLISHING"};
    mutation(document);
    auto options = ::couchbase::replace_options{};
    options.cas(couchbase_cas_from(token));
    auto [replace_error, replaced] =
        collection.replace(key, outbox_document_to_json(document), options).get();
    if (replace_error)
        throw error_for(replace_error, operation);
    return loaded(document, replaced.cas());
}
}  // namespace

LoadedOutboxMessage CouchbaseOutboxRepository::mark_published(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::EventId& event_id,
    const haven::application::persistence::PersistenceToken& token,
    std::chrono::system_clock::time_point published_at) {
    return mutate_publishing(
        *connection_,
        organization_id,
        event_id,
        token,
        [published_at](OutboxDocument& document) {
            document.status = OutboxStatus::Published;
            document.published_at = published_at;
        },
        "Couchbase Outbox mark published");
}

LoadedOutboxMessage CouchbaseOutboxRepository::release_for_retry(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::EventId& event_id,
    const haven::application::persistence::PersistenceToken& token) {
    return mutate_publishing(
        *connection_,
        organization_id,
        event_id,
        token,
        [](OutboxDocument& document) {
            document.status = OutboxStatus::Pending;
            document.published_at.reset();
        },
        "Couchbase Outbox retry release");
}

}  // namespace haven::infrastructure::persistence::couchbase
