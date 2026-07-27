/**
 * @file list_caller_reservations_query.hpp
 * @brief Defines the input query for listing a caller's reservations.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a tenant-scoped request for one caller's reservations.
 */
class ListCallerReservationsQuery final {
public:
    /**
     * @brief Constructs a caller reservation query.
     *
     * @param organization_id Organization visible to the caller.
     * @param caller_id Authenticated caller whose reservations are requested.
     */
    ListCallerReservationsQuery(
        haven::domain::OrganizationId organization_id,
        haven::domain::UserId caller_id)
        : organization_id_(std::move(organization_id)),
          caller_id_(std::move(caller_id)) {}

    /**
     * @brief Returns the organization used to scope the query.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    /**
     * @brief Returns the caller whose reservations are requested.
     */
    [[nodiscard]] const haven::domain::UserId& caller_id() const noexcept {
        return caller_id_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::UserId caller_id_;
};

}  // namespace haven::application::reservations