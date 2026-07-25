/**
 * @file reservation_conflict_policy.hpp
 * @brief Defines reservation conflict evaluation.
 */

#pragma once

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

namespace haven::domain {

/**
 * @brief Determines whether a reservation status claims the resource schedule.
 *
 * Only confirmed reservations block another reservation. Pending and terminal
 * reservations do not claim the confirmed schedule.
 *
 * @param status Reservation status to evaluate.
 *
 * @return true when the status blocks the resource schedule.
 */
[[nodiscard]] bool blocks_schedule(ReservationStatus status) noexcept;

/**
 * @brief Determines whether an existing reservation conflicts with a request.
 *
 * Conflict requires both a blocking reservation status and overlapping
 * half-open intervals. Adjacent intervals do not conflict.
 *
 * @param requested_interval Requested reservation interval.
 * @param existing_interval Existing reservation interval.
 * @param existing_status Existing reservation status.
 *
 * @return true when the existing reservation blocks the requested interval.
 */
[[nodiscard]] bool has_reservation_conflict(
    const TimeInterval& requested_interval,
    const TimeInterval& existing_interval,
    ReservationStatus existing_status) noexcept;

}  // namespace haven::domain