/**
 * @file resource.hpp
 * @brief Defines the reservable resource domain aggregate.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"

#include <compare>

namespace haven::domain {

/**
 * @brief Represents a fixed-interval resource owned by one organization.
 *
 * A resource carries the minimum business information required to determine
 * whether it may accept reservations and whether reservations require
 * approval. Reservation history and derived availability are not stored in
 * this aggregate.
 */
class Resource final {
public:
    /**
     * @brief Constructs a resource.
     *
     * @param organization_id Organization that owns the resource.
     * @param resource_id Resource identifier within the organization.
     * @param type Resource category.
     * @param status Current operational status.
     * @param requires_approval Whether reservations require approval.
     */
    Resource(
        OrganizationId organization_id,
        ResourceId resource_id,
        ResourceType type,
        ResourceStatus status,
        bool requires_approval);

    /**
     * @brief Returns the organization that owns the resource.
     *
     * @return Organization identifier.
     */
    [[nodiscard]] const OrganizationId& organization_id() const noexcept;

    /**
     * @brief Returns the resource identifier.
     *
     * @return Resource identifier.
     */
    [[nodiscard]] const ResourceId& resource_id() const noexcept;

    /**
     * @brief Returns the resource category.
     *
     * @return Resource type.
     */
    [[nodiscard]] ResourceType type() const noexcept;

    /**
     * @brief Returns the current operational status.
     *
     * @return Resource status.
     */
    [[nodiscard]] ResourceStatus status() const noexcept;

    /**
     * @brief Determines whether the resource is active.
     *
     * @return true when the resource may participate in reservation workflows.
     */
    [[nodiscard]] bool is_active() const noexcept;

    /**
     * @brief Determines whether reservations require approval.
     *
     * @return true when reservations should initially remain pending.
     */
    [[nodiscard]] bool requires_approval() const noexcept;

    /**
     * @brief Activates the resource.
     */
    void activate() noexcept;

    /**
     * @brief Deactivates the resource.
     */
    void deactivate() noexcept;

    auto operator<=>(const Resource&) const = default;

private:
    OrganizationId organization_id_;
    ResourceId resource_id_;
    ResourceType type_;
    ResourceStatus status_;
    bool requires_approval_;
};

}  // namespace haven::domain