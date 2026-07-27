/**
 * @file get_resource_calendar_handler.hpp
 * @brief Declares the GetResourceCalendar application handler.
 */

#pragma once

#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/get_resource_calendar_query.hpp"
#include "haven/application/resources/get_resource_calendar_result.hpp"
#include "haven/application/resources/resource_repository.hpp"

namespace haven::application::resources {

/**
 * @brief Retrieves a tenant-safe calendar derived from reservations.
 *
 * The handler validates the resource, queries overlapping reservations, and
 * applies defense-in-depth filtering before exposing the calendar view.
 */
class GetResourceCalendarHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository ports.
     *
     * @param resource_repository Resource catalog repository.
     * @param reservation_repository Reservation schedule repository.
     */
    GetResourceCalendarHandler(
        ResourceRepository& resource_repository,
        reservations::ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the resource calendar query.
     *
     * @param query Organization, resource, and interval information.
     * @return Resource calendar or a resource-not-found outcome.
     */
    [[nodiscard]] GetResourceCalendarResult handle(
        const GetResourceCalendarQuery& query) const;

private:
    ResourceRepository& resource_repository_;
    reservations::ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::resources