/**
 * @file reservation_status.cpp
 * @brief Implements operations for supported Haven reservation states.
 */

#include "haven/domain/value_objects/reservation_status.hpp"

namespace haven::domain {

bool is_terminal(const ReservationStatus reservation_status) noexcept {
    switch (reservation_status) {
        case ReservationStatus::PendingApproval:
        case ReservationStatus::Confirmed:
            return false;
        case ReservationStatus::Cancelled:
        case ReservationStatus::Rejected:
        case ReservationStatus::Expired:
        case ReservationStatus::Completed:
            return true;
    }

    return false;
}

std::string_view to_string(const ReservationStatus reservation_status) noexcept {
    switch (reservation_status) {
        case ReservationStatus::PendingApproval:
            return "PENDING_APPROVAL";
        case ReservationStatus::Confirmed:
            return "CONFIRMED";
        case ReservationStatus::Cancelled:
            return "CANCELLED";
        case ReservationStatus::Rejected:
            return "REJECTED";
        case ReservationStatus::Expired:
            return "EXPIRED";
        case ReservationStatus::Completed:
            return "COMPLETED";
    }

    return "UNKNOWN";
}

}  // namespace haven::domain