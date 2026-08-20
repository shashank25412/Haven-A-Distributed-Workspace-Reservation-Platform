/**
 * @file list_all_reservations_handler.hpp
 * @brief Declares the ListAllReservations application use-case handler.
 */

#pragma once

#include "haven/application/reservations/list_all_reservations_query.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Lists every reservation within one organization for administrative review.
 */
class ListAllReservationsHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit ListAllReservationsHandler(ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the tenant-scoped listing of every reservation.
     *
     * @param query Organization used to scope the listing.
     * @return All reservations belonging to the organization.
     */
    [[nodiscard]] ReservationListResult handle(const ListAllReservationsQuery& query) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations
