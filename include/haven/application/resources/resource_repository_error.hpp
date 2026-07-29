/**
 * @file resource_repository_error.hpp
 * @brief Defines infrastructure-neutral resource persistence failures.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace haven::application::resources {

/**
 * @brief Classifies failures reported by resource repository adapters.
 */
enum class ResourceRepositoryErrorCode {
    AlreadyExists,
    Authentication,
    Authorization,
    Timeout,
    Persistence
};

/**
 * @brief Reports an unexpected failure while accessing persisted resources.
 *
 * Missing resources are represented by an empty ResourceLookupResult and do
 * not produce this exception.
 */
class ResourceRepositoryError final : public std::runtime_error {
public:
    ResourceRepositoryError(ResourceRepositoryErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    /** @brief Returns the infrastructure-neutral failure classification. */
    [[nodiscard]] ResourceRepositoryErrorCode code() const noexcept {
        return code_;
    }

private:
    ResourceRepositoryErrorCode code_;
};

}  // namespace haven::application::resources
