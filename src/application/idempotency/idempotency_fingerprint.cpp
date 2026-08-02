/**
 * @file idempotency_fingerprint.cpp
 * @brief Implements the opaque logical-request fingerprint.
 */

#include "haven/application/idempotency/idempotency_fingerprint.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::idempotency {

IdempotencyFingerprint::IdempotencyFingerprint(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Idempotency fingerprint must not be empty.");
    }
}

const std::string& IdempotencyFingerprint::value() const noexcept {
    return value_;
}

}  // namespace haven::application::idempotency
