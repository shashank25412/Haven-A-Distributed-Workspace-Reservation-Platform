/**
 * @file reservation_id.cpp
 * @brief Implements the reservation identifier domain value object.
 */

#include "haven/domain/value_objects/reservation_id.hpp"

#include <stdexcept>
#include <utility>

namespace haven::domain {

ReservationId::ReservationId(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Reservation identifier must not be empty.");
    }
}

const std::string& ReservationId::value() const noexcept {
    return value_;
}

}  // namespace haven::domain