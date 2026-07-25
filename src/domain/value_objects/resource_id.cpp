/**
 * @file resource_id.cpp
 * @brief Implements the resource identifier domain value object.
 */

#include "haven/domain/value_objects/resource_id.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

ResourceId::ResourceId(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Resource identifier must not be empty.");
    }
}

const std::string& ResourceId::value() const noexcept {
    return value_;
}

}  // namespace haven::domain