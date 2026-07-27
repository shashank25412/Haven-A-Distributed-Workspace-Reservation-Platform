/**
 * @file get_resource_calendar_query.hpp
 * @brief Defines the query for retrieving a resource calendar.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

#include <utility>

namespace haven::application::resources {

/**
 * @brief Describes a tenant-scoped request for a resource calendar.
 */
class GetResourceCalendarQuery final {
public:
    /**
     * @brief Constructs a resource calendar query.
     *
     * @param organization_id Organization used to scope the query.
     * @param resource_id Resource whose calendar is requested.
     * @param interval Interval used to constrain the calendar view.
     */
    GetResourceCalendarQuery(
        haven::domain::OrganizationId organization_id,
        haven::domain::ResourceId resource_id,
        haven::domain::TimeInterval interval)
        : organization_id_(std::move(organization_id)),
          resource_id_(std::move(resource_id)),
          interval_(std::move(interval)) {}

    /**
     * @brief Returns the organization used to scope the query.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the requested resource identifier.
     */
    [[nodiscard]] const haven::domain::ResourceId& resource_id() const noexcept {
        return resource_id_;
    }

    /**
     * @brief Returns the requested calendar interval.
     */
    [[nodiscard]] const haven::domain::TimeInterval& interval() const noexcept {
        return interval_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::ResourceId resource_id_;
    haven::domain::TimeInterval interval_;
};

}  // namespace haven::application::resources