/**
 * @file list_caller_reservations_handler.hpp
 * @brief Declares the ListCallerReservations application use-case handler.
 */

#pragma once

#include "haven/application/reservations/list_caller_reservations_query.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Lists reservations belonging to one caller within an organization.
 */
class ListCallerReservationsHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit ListCallerReservationsHandler(
        ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the tenant-and-caller-scoped reservation query.
     *
     * @param query Organization and authenticated caller identifiers.
     * @return Reservations visible to the caller.
     */
    [[nodiscard]] ReservationListResult handle(
        const ListCallerReservationsQuery& query) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations