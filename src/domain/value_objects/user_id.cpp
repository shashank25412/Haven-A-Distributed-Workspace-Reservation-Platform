/**
 * @file user_id.cpp
 * @brief Implements the user identifier domain value object.
 */

#include "haven/domain/value_objects/user_id.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

UserId::UserId(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("User identifier must not be empty.");
    }
}

const std::string& UserId::value() const noexcept {
    return value_;
}

}  // namespace haven::domain