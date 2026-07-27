/**
 * @file reject_reservation_handler.hpp
 * @brief Declares the RejectReservation application use-case handler.
 */

#pragma once

#include "haven/application/reservations/reject_reservation_command.hpp"
#include "haven/application/reservations/reject_reservation_result.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Coordinates tenant-safe reservation rejection.
 *
 * The handler verifies tenant visibility and lifecycle state, invokes the
 * aggregate rejection transition, and persists the updated reservation.
 */
class RejectReservationHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit RejectReservationHandler(
        ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes reservation rejection.
     *
     * @param command Organization, reservation, rejecting user, event, and timestamp data.
     * @return Successful or failed rejection outcome.
     */
    [[nodiscard]] RejectReservationResult handle(
        const RejectReservationCommand& command) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations