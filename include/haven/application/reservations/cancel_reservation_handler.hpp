/**
 * @file cancel_reservation_handler.hpp
 * @brief Declares the CancelReservation application use-case handler.
 */

#pragma once

#include "haven/application/reservations/cancel_reservation_command.hpp"
#include "haven/application/reservations/cancel_reservation_result.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Coordinates tenant-safe reservation cancellation.
 *
 * The handler verifies tenant visibility and caller ownership before invoking
 * the aggregate lifecycle operation and persisting the updated reservation.
 */
class CancelReservationHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit CancelReservationHandler(
        ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes reservation cancellation.
     *
     * @param command Tenant, caller, reservation, event, and timestamp data.
     * @return Successful or rejected cancellation outcome.
     */
    [[nodiscard]] CancelReservationResult handle(
        const CancelReservationCommand& command) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations