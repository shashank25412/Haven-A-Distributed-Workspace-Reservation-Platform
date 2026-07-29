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
enum class ReservationKind { Standard, Maintenance };

/**
 * @brief Returns the canonical name of a reservation kind.
 *
 * @param reservation_kind Reservation kind to convert.
 *
 * @return Canonical reservation kind name.
 */
[[nodiscard]] std::string_view to_string(ReservationKind reservation_kind) noexcept;

/**
 * @brief Constructs a reservation kind from its canonical persisted name.
 *
 * @param value Canonical reservation kind name.
 * @return Parsed reservation kind.
 *
 * @throws std::invalid_argument If the value is not supported.
 */
[[nodiscard]] ReservationKind reservation_kind_from_string(std::string_view value);

}  // namespace haven::domain
