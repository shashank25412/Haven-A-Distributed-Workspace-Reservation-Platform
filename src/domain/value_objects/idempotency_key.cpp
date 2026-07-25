/**
 * @file idempotency_key.cpp
 * @brief Implements the idempotency key domain value object.
 */

#include "haven/domain/value_objects/idempotency_key.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

IdempotencyKey::IdempotencyKey(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Idempotency key must not be empty.");
    }
}

const std::string& IdempotencyKey::value() const noexcept {
    return value_;
}

}  // namespace haven::domain