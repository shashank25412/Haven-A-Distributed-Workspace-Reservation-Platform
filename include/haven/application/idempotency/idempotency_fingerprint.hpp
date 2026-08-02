/**
 * @file idempotency_fingerprint.hpp
 * @brief Defines an opaque logical-request fingerprint.
 */

#pragma once

#include <string>

namespace haven::application::idempotency {

/**
 * @brief Holds a stable encoded digest identifying one logical request.
 *
 * Fingerprints contain a lowercase hexadecimal SHA-256 digest. They must not
 * be logged because they identify caller-supplied request content.
 */
class IdempotencyFingerprint final {
public:
    /** @throws std::invalid_argument If value is empty. */
    explicit IdempotencyFingerprint(std::string value);

    [[nodiscard]] const std::string& value() const noexcept;

    bool operator==(const IdempotencyFingerprint&) const = default;

private:
    std::string value_;
};

}  // namespace haven::application::idempotency
