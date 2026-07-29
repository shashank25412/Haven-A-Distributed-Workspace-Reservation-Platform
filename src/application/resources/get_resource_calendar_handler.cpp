/**
 * @file get_resource_calendar_handler.cpp
 * @brief Implements the GetResourceCalendar application use case.
 */

#include "haven/application/resources/get_resource_calendar_handler.hpp"

#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::resources {

GetResourceCalendarHandler::GetResourceCalendarHandler(
    ResourceRepository& resource_repository,
    reservations::ReservationRepository& reservation_repository) noexcept
    : resource_repository_(resource_repository), reservation_repository_(reservation_repository) {}

GetResourceCalendarResult GetResourceCalendarHandler::handle(
    const GetResourceCalendarQuery& query) const {
    HVN_TRACE_SCOPE();

    const auto resource =
        resource_repository_.find_by_id(query.organization_id(), query.resource_id());

    if (!resource.has_value() ||
        resource->aggregate().organization_id() != query.organization_id()) {
        HVN_WARN_LOG("Resource calendar query failed because the resource is unavailable");
        return GetResourceCalendarResult::resource_not_found();
    }

    auto calendar = reservation_repository_.find_by_resource_and_interval(
        query.organization_id(), query.resource_id(), query.interval());

    const auto first_hidden_reservation = std::remove_if(
        calendar.begin(), calendar.end(), [&query](const haven::domain::Reservation& reservation) {
            return reservation.organization_id() != query.organization_id() ||
                   reservation.resource_id() != query.resource_id() ||
                   !reservation.interval().overlaps(query.interval());
        });

    if (first_hidden_reservation != calendar.end()) {
        HVN_WARN_LOG(
            "Reservation repository returned entries outside the requested calendar scope");
        calendar.erase(first_hidden_reservation, calendar.end());
    }

    return GetResourceCalendarResult::found(std::move(calendar));
}

}  // namespace haven::application::resources
