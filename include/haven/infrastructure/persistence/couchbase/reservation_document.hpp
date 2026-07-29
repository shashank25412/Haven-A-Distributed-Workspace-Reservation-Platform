/**
 * @file reservation_document.hpp
 * @brief Defines the Couchbase persistence representation of a reservation.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tao/json/value.hpp>

namespace haven::infrastructure::persistence::couchbase {

/** @brief Current supported reservation document schema version. */
inline constexpr std::uint64_t kReservationDocumentSchemaVersion{1};

/** @brief Canonical persisted document type for reservations. */
inline constexpr const char* kReservationDocumentType{"reservation"};

/**
 * @brief Persisted approval details for an approved reservation.
 *
 * Timestamps use a fixed ISO-8601 UTC representation with nanosecond precision.
 */
struct ReservationApprovalDocument {
    std::string approved_by;
    std::string approved_at;

    bool operator==(const ReservationApprovalDocument&) const = default;
};

/**
 * @brief Infrastructure-owned reservation representation stored in Couchbase.
 *
 * Purpose text is preserved exactly. Start, end, and approval timestamps use a
 * fixed ISO-8601 UTC representation with nanosecond precision.
 */
struct ReservationDocument {
    std::uint64_t schema_version;
    std::string reservation_id;
    std::string organization_id;
    std::string resource_id;
    std::string created_by;
    std::string start_time;
    std::string end_time;
    std::string purpose;
    std::string status;
    std::string kind;
    std::optional<ReservationApprovalDocument> approval;
    std::uint64_t version;

    bool operator==(const ReservationDocument&) const = default;
};

/**
 * @brief Serializes a validated reservation document to JSON.
 *
 * Unknown schema versions are rejected.
 */
[[nodiscard]] tao::json::value reservation_document_to_json(const ReservationDocument& document);

/**
 * @brief Deserializes and validates a reservation document from JSON.
 *
 * Unknown additive fields are ignored and unknown schema versions are rejected.
 */
[[nodiscard]] ReservationDocument reservation_document_from_json(const tao::json::value& json);

}  // namespace haven::infrastructure::persistence::couchbase
