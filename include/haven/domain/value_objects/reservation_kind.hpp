/**
 * @file reservation_kind.hpp
 * @brief Defines the supported Haven reservation kinds.
 */

#pragma once

#include <string_view>

namespace haven::domain {

/**
 * @brief Identifies the business category of a reservation.
 *
 * Maintenance reservations may be subject to different duration policies.
 * Authorization to create one must be established separately and must not be
 * inferred from the reservation purpose.
 */
enum class ReservationKind {
    Standard,
    Maintenance
};

/**
 * @brief Returns the canonical name of a reservation kind.
 *
 * @param reservation_kind Reservation kind to convert.
 *
 * @return Canonical reservation kind name.
 */
[[nodiscard]] std::string_view to_string(ReservationKind reservation_kind) noexcept;

}  // namespace haven::domain