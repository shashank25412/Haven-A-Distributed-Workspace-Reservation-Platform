/**
 * @file list_pending_approvals_query.hpp
 * @brief Defines the input query for listing pending reservation approvals.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"

#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a tenant-scoped request for pending approvals.
 */
class ListPendingApprovalsQuery final {
public:
    /**
     * @brief Constructs a pending approval query.
     *
     * @param organization_id Organization whose approval queue is requested.
     */
    explicit ListPendingApprovalsQuery(
        haven::domain::OrganizationId organization_id)
        : organization_id_(std::move(organization_id)) {}

    /**
     * @brief Returns the organization used to scope the approval queue.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

private:
    haven::domain::OrganizationId organization_id_;
};

}  // namespace haven::application::reservations