/**
 * @file approve_reservation_handler.hpp
 * @brief Declares the ApproveReservation application use-case handler.
 */

#pragma once

#include "haven/application/reservations/approve_reservation_command.hpp"
#include "haven/application/reservations/approve_reservation_result.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Coordinates tenant-safe reservation approval.
 *
 * The handler verifies tenant visibility and lifecycle state, checks the
 * resource schedule, invokes the aggregate transition, and persists the
 * updated reservation.
 */
class ApproveReservationHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit ApproveReservationHandler(
        ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes reservation approval.
     *
     * @param command Organization, reservation, approver, event, and timestamp data.
     * @return Successful or rejected approval outcome.
     */
    [[nodiscard]] ApproveReservationResult handle(
        const ApproveReservationCommand& command) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations