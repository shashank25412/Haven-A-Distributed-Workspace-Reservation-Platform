/**
 * @file reservation_document_validator.hpp
 * @brief Declares validation for persisted reservation documents.
 */

#pragma once

#include "haven/infrastructure/persistence/couchbase/reservation_document.hpp"

#include <chrono>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Converts a time point to fixed ISO-8601 UTC nanosecond text.
 */
[[nodiscard]] std::string reservation_timestamp_to_string(
    std::chrono::system_clock::time_point timestamp);

/**
 * @brief Parses fixed ISO-8601 UTC nanosecond text.
 *
 * @throws std::invalid_argument If the timestamp is malformed.
 */
[[nodiscard]] std::chrono::system_clock::time_point reservation_timestamp_from_string(
    std::string_view timestamp);

/**
 * @brief Validates structural persistence constraints.
 *
 * Purpose is intentionally permitted to be empty or whitespace-only. Unknown
 * schema versions and malformed timestamps are rejected.
 */
void validate_reservation_document(const ReservationDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase
