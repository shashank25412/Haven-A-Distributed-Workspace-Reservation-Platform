/**
 * @file organization_document.hpp
 * @brief Defines the Couchbase persistence representation of an organization.
 */

#pragma once

#include <tao/json/value.hpp>

#include <cstdint>
#include <string>

namespace haven::infrastructure::persistence::couchbase {

/** @brief Current persisted schema version for organization documents. */
inline constexpr std::uint64_t kOrganizationDocumentSchemaVersion{1};

/** @brief Canonical persisted document type for organizations. */
inline constexpr const char* kOrganizationDocumentType{"organization"};

/**
 * @brief Represents an organization document stored in Couchbase.
 */
struct OrganizationDocument {
    std::uint64_t schema_version;
    std::string organization_id;
    std::string name;
};

/**
 * @brief Serializes an organization document into Couchbase JSON.
 */
[[nodiscard]] tao::json::value organization_document_to_json(
    const OrganizationDocument& document);

/**
 * @brief Parses and validates a Couchbase organization document.
 *
 * @throws std::invalid_argument If the document type, schema version, or
 * persisted field values are invalid.
 * @throws std::exception If a required JSON field is missing or has an
 * incompatible type.
 */
[[nodiscard]] OrganizationDocument organization_document_from_json(
    const tao::json::value& json);

}  // namespace haven::infrastructure::persistence::couchbase
