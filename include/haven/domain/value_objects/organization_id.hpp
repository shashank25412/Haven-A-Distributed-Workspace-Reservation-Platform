/**
 * @file organization_id.hpp
 * @brief Defines the organization identifier domain value object.
 */

#pragma once

#include <compare>
#include <string>

namespace haven::domain {

/**
 * @brief Identifies an organization and establishes a tenant boundary.
 *
 * Organization identifiers are treated as opaque values. Construction rejects
 * an empty identifier but does not impose a transport-specific format.
 */
class OrganizationId final {
public:
    /**
     * @brief Constructs an organization identifier.
     *
     * @param value Opaque organization identifier value.
     *
     * @throws std::invalid_argument when the supplied value is empty.
     */
    explicit OrganizationId(std::string value);

    /**
     * @brief Returns the underlying opaque identifier value.
     *
     * @return Reference to the stored identifier.
     */
    [[nodiscard]] const std::string& value() const noexcept;

    auto operator<=>(const OrganizationId&) const = default;

private:
    std::string value_;
};

}  // namespace haven::domain