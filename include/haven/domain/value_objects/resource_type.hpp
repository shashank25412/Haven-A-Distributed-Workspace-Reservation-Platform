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

}  // namespace haven::domain