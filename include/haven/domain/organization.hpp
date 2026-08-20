/**
 * @file organization.hpp
 * @brief Defines the organization directory aggregate.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"

#include <compare>
#include <string>

namespace haven::domain {

/**
 * @brief Represents a tenant organization and its display name.
 *
 * Organization is a read-oriented aggregate used to present a human-readable
 * name alongside the opaque OrganizationId tenant boundary.
 */
class Organization final {
public:
    /**
     * @brief Constructs an organization.
     *
     * @param organization_id Tenant identifier.
     * @param name Display name.
     *
     * @throws std::invalid_argument If name is empty.
     */
    Organization(OrganizationId organization_id, std::string name);

    /** @brief Returns the tenant identifier. */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /** @brief Returns the display name. */
    [[nodiscard]] const std::string& name() const noexcept;

    auto operator<=>(const Organization&) const = default;

private:
    OrganizationId organization_id_;
    std::string name_;
};

}  // namespace haven::domain
