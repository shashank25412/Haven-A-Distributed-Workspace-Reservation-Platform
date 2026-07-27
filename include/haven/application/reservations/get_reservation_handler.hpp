/**
 * @file get_reservation_handler.hpp
 * @brief Declares the GetReservation application use-case handler.
 */

#pragma once

#include "haven/application/reservations/get_reservation_query.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Retrieves one reservation while preserving tenant isolation.
 */
class GetReservationHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit GetReservationHandler(ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the tenant-scoped reservation lookup.
     *
     * A missing reservation and a reservation owned by another organization
     * produce the same empty result.
     *
     * @param query Tenant and reservation identifiers for the lookup.
     * @return The visible reservation or an empty result.
     */
    [[nodiscard]] ReservationLookupResult handle(
        const GetReservationQuery& query) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations