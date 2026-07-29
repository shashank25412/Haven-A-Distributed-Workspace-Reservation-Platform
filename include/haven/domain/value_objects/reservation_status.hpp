/**
 * @file reservation_status.hpp
 * @brief Defines the supported Haven reservation states.
 */

#pragma once

#include <string_view>

namespace haven::domain {

/**
 * @brief Represents the current lifecycle state of a reservation.
 *
 * Reservation state changes must occur through named domain operations such as
 * approve, reject, cancel, extend, expire, and complete. A generic status
 * setter must not be exposed by the reservation aggregate.
 */
enum class ReservationStatus {
    PendingApproval,
    Confirmed,
    Cancelled,
    Rejected,
    Expired,
    Completed
};

/**
 * @brief Determines whether a reservation status is terminal.
 *
 * Terminal reservations cannot transition to another lifecycle state.
 *
 * @param reservation_status Reservation status to evaluate.
 *
 * @return true when the status is terminal; otherwise, false.
 */
[[nodiscard]] bool is_terminal(ReservationStatus reservation_status) noexcept;

/**
 * @brief Returns the canonical name of a reservation status.
 *
 * @param reservation_status Reservation status to convert.
 *
 * @return Canonical reservation status name.
 */
[[nodiscard]] std::string_view to_string(ReservationStatus reservation_status) noexcept;

/**
 * @brief Constructs a reservation status from its canonical persisted name.
 *
 * @param value Canonical reservation status name.
 * @return Parsed reservation status.
 *
 * @throws std::invalid_argument If the value is not supported.
 */
[[nodiscard]] ReservationStatus reservation_status_from_string(std::string_view value);

}  // namespace haven::domain
