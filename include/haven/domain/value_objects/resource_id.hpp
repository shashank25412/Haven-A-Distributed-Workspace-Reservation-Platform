/**
 * @file resource_id.hpp
 * @brief Defines the resource identifier domain value object.
 */

#pragma once

#include <compare>
#include <string>

namespace haven::domain {

/**
 * @brief Identifies a reservable resource within an organization.
 *
 * Resource identifiers are opaque values. Tenant context is supplied
 * separately whenever resources are loaded or persisted.
 */
class ResourceId final {
public:
    /**
     * @brief Constructs a resource identifier.
     *
     * @param value Opaque resource identifier value.
     *
     * @throws std::invalid_argument when the supplied value is empty.
     */
    explicit ResourceId(std::string value);

    /**
     * @brief Returns the underlying opaque identifier value.
     *
     * @return Reference to the stored identifier.
     */
    [[nodiscard]] const std::string& value() const noexcept;

    auto operator<=>(const ResourceId&) const = default;

private:
    std::string value_;
};

}  // namespace haven::domain