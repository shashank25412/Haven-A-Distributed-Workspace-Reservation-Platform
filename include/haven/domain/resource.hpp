/**
 * @file resource.hpp
 * @brief Defines the reservable resource domain aggregate.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/version.hpp"

#include <compare>
#include <cstdint>
#include <string>

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
        bool requires_approval,
        std::uint32_t total_units = 1,
        std::string address = "");

    /**
     * @brief Restores a resource from previously persisted state.
     *
     * @param organization_id Organization that owns the resource.
     * @param resource_id Persisted resource identifier.
     * @param name Persisted display name.
     * @param description Persisted description.
     * @param type Persisted resource category.
     * @param status Persisted operational status.
     * @param requires_approval Persisted approval requirement.
     * @param version Persistence-neutral optimistic concurrency version.
     *
     * @return Rehydrated resource.
     *
     * @throws std::invalid_argument If name is empty or version is zero.
     */
    [[nodiscard]] static Resource rehydrate(
        OrganizationId organization_id,
        ResourceId resource_id,
        std::string name,
        std::string description,
        ResourceType type,
        ResourceStatus status,
        bool requires_approval,
        Version version,
        std::uint32_t total_units = 1,
        std::string address = "");

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

    /** @brief Returns the persisted display name. */
    [[nodiscard]] const std::string& name() const noexcept;

    /** @brief Returns the persisted description. */
    [[nodiscard]] const std::string& description() const noexcept;

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

    /** @brief Returns the optimistic concurrency version. */
    [[nodiscard]] Version version() const noexcept;

    /**
     * @brief Returns how many interchangeable units this resource represents.
     *
     * Most resources (a single meeting room, a single desk) have exactly one
     * unit. Bulk/pooled resources (a parking lot, a block of shared desks, a
     * dining hall's tables) advertise more than one so several reservations
     * can hold the same resource concurrently up to this capacity.
     */
    [[nodiscard]] std::uint32_t total_units() const noexcept;

    /** @brief Returns the physical street address, or empty when not recorded. */
    [[nodiscard]] const std::string& address() const noexcept;

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
    Resource(
        OrganizationId organization_id,
        ResourceId resource_id,
        std::string name,
        std::string description,
        ResourceType type,
        ResourceStatus status,
        bool requires_approval,
        Version version,
        std::uint32_t total_units,
        std::string address);

    OrganizationId organization_id_;
    ResourceId resource_id_;
    std::string name_;
    std::string description_;
    ResourceType type_;
    ResourceStatus status_;
    bool requires_approval_;
    Version version_;
    std::uint32_t total_units_;
    std::string address_;
};

}  // namespace haven::domain
