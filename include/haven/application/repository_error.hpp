/**
 * @file repository_error.hpp
 * @brief Defines generic application repository failures.
 */

#pragma once

#include <stdexcept>
#include <string>

namespace haven::application {

/** @brief Classifies failures reported by repository adapters. */
enum class RepositoryErrorCode {
    AlreadyExists,
    ConcurrencyConflict,
    Authentication,
    Authorization,
    Timeout,
    Persistence
};

/**
 * @brief Reports an unexpected failure while accessing persisted aggregates.
 *
 * Missing point lookups remain represented by an empty optional.
 */
class RepositoryError final : public std::runtime_error {
public:
    RepositoryError(RepositoryErrorCode code, std::string message);

    /** @brief Returns the infrastructure-neutral failure classification. */
    [[nodiscard]] RepositoryErrorCode code() const noexcept;

private:
    RepositoryErrorCode code_;
};

}  // namespace haven::application
