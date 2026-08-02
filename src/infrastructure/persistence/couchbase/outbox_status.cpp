/**
 * @file outbox_status.cpp
 * @brief Implements persisted Outbox publication-state conversion.
 */

#include "haven/infrastructure/persistence/couchbase/outbox_status.hpp"

#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase {

std::string_view to_string(const OutboxStatus status) {
    switch (status) {
        case OutboxStatus::Pending:
            return "PENDING";
        case OutboxStatus::Publishing:
            return "PUBLISHING";
        case OutboxStatus::Published:
            return "PUBLISHED";
    }
    throw std::invalid_argument("Unsupported Outbox status");
}

OutboxStatus outbox_status_from_string(const std::string_view value) {
    if (value == "PENDING")
        return OutboxStatus::Pending;
    if (value == "PUBLISHING")
        return OutboxStatus::Publishing;
    if (value == "PUBLISHED")
        return OutboxStatus::Published;
    throw std::invalid_argument("Unknown Outbox status");
}

}  // namespace haven::infrastructure::persistence::couchbase
