/**
 * @file get_reservation_query.hpp
 * @brief Defines the input query for retrieving one tenant-scoped reservation.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"

#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a request to retrieve one reservation within an organization.
 */
class GetReservationQuery final {
public:
    /**
     * @brief Constructs a tenant-scoped reservation query.
     *
     * @param organization_id Organization visible to the caller.
     * @param reservation_id Reservation identifier requested by the caller.
     */
    GetReservationQuery(
        haven::domain::OrganizationId organization_id,
        haven::domain::ReservationId reservation_id)
        : organization_id_(std::move(organization_id)),
          reservation_id_(std::move(reservation_id)) {}

    /**
     * @brief Returns the organization used to scope the lookup.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the requested reservation identifier.
     */
    [[nodiscard]] const haven::domain::ReservationId& reservation_id() const noexcept {
        return reservation_id_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ReservationId reservation_id_;
};

}  // namespace haven::application::reservations