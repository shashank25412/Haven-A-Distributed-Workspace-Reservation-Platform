/**
 * @file get_resource_calendar_result.hpp
 * @brief Defines outcomes produced by the GetResourceCalendar use case.
 */

#pragma once

#include "haven/application/reservations/reservation_repository.hpp"

#include <utility>

namespace haven::application::resources {

/**
 * @brief Identifies the outcome of a resource calendar query.
 */
enum class GetResourceCalendarStatus {
    FOUND,
    RESOURCE_NOT_FOUND
};

/**
 * @brief Represents the result of retrieving a resource calendar.
 */
class GetResourceCalendarResult final {
public:
    /**
     * @brief Creates a successful calendar result.
     *
     * @param reservations Reservations forming the derived calendar view.
     */
    [[nodiscard]] static GetResourceCalendarResult found(
        reservations::ReservationListResult reservations) {
        return GetResourceCalendarResult{
            GetResourceCalendarStatus::FOUND,
            std::move(reservations)};
    }

    /**
     * @brief Creates a result indicating that the resource is unavailable.
     */
    [[nodiscard]] static GetResourceCalendarResult resource_not_found() {
        return GetResourceCalendarResult{
            GetResourceCalendarStatus::RESOURCE_NOT_FOUND,
            {}};
    }

    /**
     * @brief Returns the calendar query outcome.
     */
    [[nodiscard]] GetResourceCalendarStatus status() const noexcept {
        return status_;
    }

    /**
     * @brief Returns the reservations forming the calendar view.
     */
    [[nodiscard]] const reservations::ReservationListResult& reservations() const noexcept {
        return reservations_;
    }

private:
    GetResourceCalendarResult(
        const GetResourceCalendarStatus status,
        reservations::ReservationListResult reservations)
        : status_(status),
          reservations_(std::move(reservations)) {}

    GetResourceCalendarStatus status_;
    reservations::ReservationListResult reservations_;
};

}  // namespace haven::application::resources