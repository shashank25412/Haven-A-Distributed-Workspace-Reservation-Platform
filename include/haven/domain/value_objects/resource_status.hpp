/**
 * @file resource_status.hpp
 * @brief Defines the supported Haven resource states.
 */

#pragma once

#include <string_view>

namespace haven::domain {

/**
 * @brief Represents the operational state of a reservable resource.
 *
 * Only active resources may be considered available for new reservations.
 * Priority and approval requirements are separate resource properties and must
 * not be represented through this status.
 */
enum class ResourceStatus {
    Active,
    Inactive
};

/**
 * @brief Returns the canonical name of a resource status.
 *
 * @param resource_status Resource status to convert.
 *
 * @return Canonical resource status name.
 */
[[nodiscard]] std::string_view to_string(ResourceStatus resource_status) noexcept;

/**
 * @brief Constructs a resource status from its canonical persisted name.
 *
 * @param value Canonical resource status name.
 *
 * @return Parsed resource status.
 *
 * @throws std::invalid_argument If the value is not supported.
 */
[[nodiscard]] ResourceStatus resource_status_from_string(
    std::string_view value);

}  // namespace haven::domain
