/**
 * @file reservation_kind.cpp
 * @brief Implements conversion for supported Haven reservation kinds.
 */

#include "haven/domain/value_objects/reservation_kind.hpp"

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

}  // namespace haven::domain