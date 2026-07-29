/**
 * @file reservation_kind.cpp
 * @brief Implements conversion for supported Haven reservation kinds.
 */

#include "haven/domain/value_objects/reservation_kind.hpp"

#include <stdexcept>
#include <string>

namespace haven::domain {

std::string_view to_string(const ReservationKind reservation_kind) noexcept {
    switch (reservation_kind) {
        case ReservationKind::Standard:
            return "STANDARD";
        case ReservationKind::Maintenance:
            return "MAINTENANCE";
    }

    return "UNKNOWN";
}

ReservationKind reservation_kind_from_string(const std::string_view value) {
    if (value == "STANDARD") {
        return ReservationKind::Standard;
    }
    if (value == "MAINTENANCE") {
        return ReservationKind::Maintenance;
    }

    throw std::invalid_argument("Unsupported reservation kind: " + std::string{value});
}

}  // namespace haven::domain
