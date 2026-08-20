/**
 * @file list_all_reservations_query.hpp
 * @brief Defines the input query for listing every reservation in an organization.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"

#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes an administrative request for all reservations in one organization.
 */
class ListAllReservationsQuery final {
public:
    /**
     * @brief Constructs an all-reservations query.
     *
     * @param organization_id Organization whose reservations should be returned.
     */
    explicit ListAllReservationsQuery(haven::domain::OrganizationId organization_id)
        : organization_id_(std::move(organization_id)) {}

    /**
     * @brief Returns the organization used to scope the query.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

private:
    haven::domain::OrganizationId organization_id_;
};

}  // namespace haven::application::reservations
