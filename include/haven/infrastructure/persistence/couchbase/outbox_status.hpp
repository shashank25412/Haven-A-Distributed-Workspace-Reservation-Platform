/**
 * @file outbox_status.hpp
 * @brief Defines persisted Outbox publication states.
 */

#pragma once

#include <string_view>

namespace haven::infrastructure::persistence::couchbase {

enum class OutboxStatus {
    Pending,
    Publishing,
    Published,
};

[[nodiscard]] std::string_view to_string(OutboxStatus status);
[[nodiscard]] OutboxStatus outbox_status_from_string(std::string_view value);

}  // namespace haven::infrastructure::persistence::couchbase
