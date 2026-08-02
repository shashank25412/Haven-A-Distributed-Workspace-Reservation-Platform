/**
 * @file idempotency_repository_error.hpp
 * @brief Defines application contract violations for idempotency persistence.
 */

#pragma once

#include <stdexcept>
#include <string>

namespace haven::application::idempotency {

/** @brief Classifies invalid terminal-transition repository operations. */
enum class IdempotencyRepositoryErrorCode {
    MissingRecord,
    FingerprintMismatch,
    TerminalConflict,
    InvalidRecord,
    InvalidSnapshot
};

/**
 * @brief Reports a violated idempotency repository contract.
 *
 * Claim contention is not an error and is represented by IdempotencyClaimResult.
 * Messages deliberately omit raw fingerprints and caller keys.
 */
class IdempotencyRepositoryError final : public std::logic_error {
public:
    IdempotencyRepositoryError(IdempotencyRepositoryErrorCode code, std::string message)
        : std::logic_error(std::move(message)), code_(code) {}

    [[nodiscard]] IdempotencyRepositoryErrorCode code() const noexcept {
        return code_;
    }

private:
    IdempotencyRepositoryErrorCode code_;
};

}  // namespace haven::application::idempotency
