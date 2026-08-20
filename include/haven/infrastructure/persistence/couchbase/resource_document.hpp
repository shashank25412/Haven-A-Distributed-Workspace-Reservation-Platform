/**
 * @file resource_document.hpp
 * @brief Defines the Couchbase persistence representation of a resource.
 */

#pragma once

#include <tao/json/value.hpp>

#include <cstdint>
#include <string>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Current persisted schema version for resource documents.
 */
inline constexpr std::uint64_t kResourceDocumentSchemaVersion{1};

/**
 * @brief Canonical persisted document type for resources.
 */
inline constexpr const char* kResourceDocumentType{"resource"};

/**
 * @brief Represents a resource document stored in Couchbase.
 *
 * This infrastructure-owned type reflects the persisted JSON structure and
 * remains separate from the domain Resource aggregate.
 */
struct ResourceDocument {
    std::uint64_t schema_version;
    std::string resource_id;
    std::string organization_id;
    std::string name;
    std::string description;
    std::string resource_type;
    std::string status;
    bool requires_approval;
    std::uint64_t version;
    /** Interchangeable unit capacity; older documents without this field default to 1. */
    std::uint32_t total_units{1};
    /** Physical street address; older documents without this field default to empty. */
    std::string address{};
};

/**
 * @brief Serializes a resource document into Couchbase JSON.
 *
 * @param document Resource document to serialize.
 * @return JSON representation of the resource document.
 */
[[nodiscard]] tao::json::value resource_document_to_json(
    const ResourceDocument& document);

/**
 * @brief Parses and validates a Couchbase resource document.
 *
 * The operation validates the persisted document type, extracts all required
 * fields, and applies resource-document structural validation.
 *
 * Unknown additional JSON fields are ignored to support additive schema
 * evolution.
 *
 * @param json JSON representation read from Couchbase.
 * @return Parsed and validated resource document.
 *
 * @throws std::invalid_argument If the document type, schema version, or
 * persisted field values are invalid.
 * @throws std::exception If a required JSON field is missing or has an
 * incompatible type.
 */
[[nodiscard]] ResourceDocument resource_document_from_json(
    const tao::json::value& json);

}  // namespace haven::infrastructure::persistence::couchbase
