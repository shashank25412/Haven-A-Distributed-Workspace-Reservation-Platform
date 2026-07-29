/**
 * @file resource_type.hpp
 * @brief Defines the supported Haven resource types.
 */

#pragma once

#include <string_view>

namespace haven::domain {

/**
 * @brief Identifies the category of a reservable resource.
 *
 * Every supported resource type follows the same fixed-time reservation model.
 * Type-specific behaviour should be introduced only when required by an
 * explicit business rule.
 */
enum class ResourceType {
    MeetingRoom,
    OfficeDesk,
    ParkingSlot,
    HotelRoom,
    GameZone
};

/**
 * @brief Returns the canonical name of a resource type.
 *
 * @param resource_type Resource type to convert.
 *
 * @return Canonical resource type name.
 */
[[nodiscard]] std::string_view to_string(ResourceType resource_type) noexcept;

/**
 * @brief Constructs a resource type from its canonical persisted name.
 *
 * @param value Canonical resource type name.
 *
 * @return Parsed resource type.
 *
 * @throws std::invalid_argument If the value is not supported.
 */
[[nodiscard]] ResourceType resource_type_from_string(std::string_view value);

}  // namespace haven::domain
