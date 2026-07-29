/**
 * @file couchbase_document_key.cpp
 * @brief Implements tenant-scoped Couchbase document-key generation.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_document_key.hpp"

#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

namespace {

constexpr std::string_view kResourceDocumentPrefix{"resource::"};
constexpr std::string_view kReservationDocumentPrefix{"reservation::"};
constexpr std::string_view kSeparator{"::"};

[[nodiscard]] std::string tenant_scoped_document_key(
    const std::string_view prefix,
    const std::string_view organization_id,
    const std::string_view entity_id) {
    std::string document_key;

    document_key.reserve(
        prefix.size() +
        organization_id.size() +
        kSeparator.size() +
        entity_id.size());

    document_key.append(prefix);
    document_key.append(organization_id);
    document_key.append(kSeparator);
    document_key.append(entity_id);

    return document_key;
}

}  // namespace

std::string resource_document_key(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) {
    return tenant_scoped_document_key(
        kResourceDocumentPrefix,
        organization_id.value(),
        resource_id.value());
}

std::string reservation_document_key(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id) {
    return tenant_scoped_document_key(
        kReservationDocumentPrefix,
        organization_id.value(),
        reservation_id.value());
}

}  // namespace haven::infrastructure::persistence::couchbase