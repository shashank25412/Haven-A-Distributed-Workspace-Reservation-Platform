#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tao/json/value.hpp>

namespace haven::infrastructure::persistence::couchbase {

inline constexpr std::uint64_t kIdempotencyDocumentSchemaVersion{1};
inline constexpr const char* kIdempotencyDocumentType{"idempotency"};

struct IdempotencyResultDocument {
    std::string creation_status;
    std::optional<std::string> reservation_id;
    std::optional<std::string> resource_id;
    std::optional<std::string> reservation_status;
    std::optional<std::string> reservation_kind;
    std::optional<std::uint64_t> initial_version;
    std::optional<std::string> created_at;
    bool operator==(const IdempotencyResultDocument&) const = default;
};

struct IdempotencyDocument {
    std::uint64_t schema_version;
    std::string organization_id;
    std::string creator_id;
    std::string operation;
    std::string idempotency_key;
    std::string fingerprint;
    std::string status;
    std::string reservation_id;
    std::string created_event_id;
    std::string confirmed_event_id;
    std::string approval_requested_event_id;
    std::string created_at;
    std::optional<IdempotencyResultDocument> result;
    bool operator==(const IdempotencyDocument&) const = default;
};

[[nodiscard]] tao::json::value idempotency_document_to_json(const IdempotencyDocument& document);
[[nodiscard]] IdempotencyDocument idempotency_document_from_json(const tao::json::value& json);

}  // namespace haven::infrastructure::persistence::couchbase
