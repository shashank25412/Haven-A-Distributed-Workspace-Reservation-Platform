/**
 * @file event_id.cpp
 * @brief Implements the event identifier domain value object.
 */

#include "haven/domain/value_objects/event_id.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

EventId::EventId(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Event identifier must not be empty.");
    }
}

const std::string& EventId::value() const noexcept {
    return value_;
}

}  // namespace haven::domain