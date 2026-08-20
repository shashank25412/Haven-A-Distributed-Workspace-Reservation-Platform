/**
 * @file list_decided_approvals_query.hpp
 * @brief Defines the input query for listing decided reservation approvals.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"

#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a tenant-scoped request for previously decided approvals.
 */
class ListDecidedApprovalsQuery final {
public:
    /**
     * @brief Constructs a decided approval query.
     *
     * @param organization_id Organization whose decision history is requested.
     */
    explicit ListDecidedApprovalsQuery(
        haven::domain::OrganizationId organization_id)
        : organization_id_(std::move(organization_id)) {}

    /**
     * @brief Returns the organization used to scope the decision history.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

private:
    haven::domain::OrganizationId organization_id_;
};

}  // namespace haven::application::reservations
