/**
 * @file purpose.cpp
 * @brief Implements the reservation purpose domain value object.
 */

#include "haven/domain/value_objects/purpose.hpp"

#include <utility>

namespace haven::domain {

Purpose::Purpose(std::string value) : value_(std::move(value)) {
}

const std::string& Purpose::value() const noexcept {
    return value_;
}

}  // namespace haven::domain