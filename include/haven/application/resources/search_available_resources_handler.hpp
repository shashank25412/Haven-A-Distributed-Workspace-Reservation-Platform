/**
 * @file search_available_resources_handler.hpp
 * @brief Declares the SearchAvailableResources application handler.
 */

#pragma once

#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/resource_availability.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/application/resources/search_available_resources_query.hpp"

namespace haven::application::resources {

/**
 * @brief Searches for active resources without reservation conflicts.
 *
 * The handler combines the resource catalog port with the reservation
 * repository port to derive availability for a requested interval. Bulk
 * resources (total_units > 1) remain in the result, with reduced remaining
 * capacity, as long as at least one unit is still bookable.
 */
class SearchAvailableResourcesHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository ports.
     *
     * @param resource_repository Resource catalog repository.
     * @param reservation_repository Reservation schedule repository.
     */
    SearchAvailableResourcesHandler(
        ResourceRepository& resource_repository,
        reservations::ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the availability search.
     *
     * @param query Organization, resource type, and requested interval.
     * @return Active matching resources with remaining unit availability.
     */
    [[nodiscard]] ResourceAvailabilityResult handle(
        const SearchAvailableResourcesQuery& query) const;

private:
    ResourceRepository& resource_repository_;
    reservations::ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::resources