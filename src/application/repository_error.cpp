/**
 * @file repository_error.cpp
 * @brief Implements generic application repository failures.
 */

#include "haven/application/repository_error.hpp"

#include <utility>

namespace haven::application {

RepositoryError::RepositoryError(const RepositoryErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

RepositoryErrorCode RepositoryError::code() const noexcept {
    return code_;
}

}  // namespace haven::application
