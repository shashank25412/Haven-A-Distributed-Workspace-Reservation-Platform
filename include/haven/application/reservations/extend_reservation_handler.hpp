/**
 * @file extend_reservation_handler.hpp
 * @brief Declares the ExtendReservation application use-case handler.
 */

#pragma once

#include "haven/application/reservations/extend_reservation_command.hpp"
#include "haven/application/reservations/extend_reservation_result.hpp"
#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/domain/policies/reservation_policy.hpp"

namespace haven::application::reservations {

/**
 * @brief Coordinates tenant-safe reservation extension.
 *
 * The handler verifies visibility and ownership, evaluates interval policy,
 * checks conflicts excluding the current reservation, invokes the aggregate
 * lifecycle operation, and persists the updated aggregate.
 */
class ExtendReservationHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository and policy.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     * @param reservation_policy Domain policy governing reservation intervals.
     */
    ExtendReservationHandler(
        ReservationRepository& reservation_repository,
        const haven::domain::ReservationPolicy& reservation_policy) noexcept;

    /**
     * @brief Executes reservation extension.
     *
     * @param command Tenant, caller, interval, event, and timestamp data.
     * @return Successful or rejected extension outcome.
     */
    [[nodiscard]] ExtendReservationResult handle(
        const ExtendReservationCommand& command) const;

private:
    ReservationRepository& reservation_repository_;
    const haven::domain::ReservationPolicy& reservation_policy_;
};

}  // namespace haven::application::reservations
