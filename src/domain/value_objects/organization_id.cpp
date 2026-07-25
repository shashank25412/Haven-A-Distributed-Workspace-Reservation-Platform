/**
 * @file organization_id.cpp
 * @brief Implements the organization identifier domain value object.
 */

#include "haven/domain/value_objects/organization_id.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

OrganizationId::OrganizationId(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Organization identifier must not be empty.");
    }
}

const std::string& OrganizationId::value() const noexcept {
    return value_;
}

}  // namespace haven::domain