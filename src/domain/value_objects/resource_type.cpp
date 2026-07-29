/**
 * @file resource_type.cpp
 * @brief Implements conversion for supported Haven resource types.
 */

#include "haven/domain/value_objects/resource_type.hpp"

#include <stdexcept>
#include <string>

namespace haven::domain {

std::string_view to_string(const ResourceType resource_type) noexcept {
    switch (resource_type) {
        case ResourceType::MeetingRoom:
            return "MEETING_ROOM";
        case ResourceType::OfficeDesk:
            return "OFFICE_DESK";
        case ResourceType::ParkingSlot:
            return "PARKING_SLOT";
        case ResourceType::HotelRoom:
            return "HOTEL_ROOM";
        case ResourceType::GameZone:
            return "GAME_ZONE";
    }

    return "UNKNOWN";
}

ResourceType resource_type_from_string(const std::string_view value) {
    if (value == "MEETING_ROOM") {
        return ResourceType::MeetingRoom;
    }
    if (value == "OFFICE_DESK") {
        return ResourceType::OfficeDesk;
    }
    if (value == "PARKING_SLOT") {
        return ResourceType::ParkingSlot;
    }
    if (value == "HOTEL_ROOM") {
        return ResourceType::HotelRoom;
    }
    if (value == "GAME_ZONE") {
        return ResourceType::GameZone;
    }

    throw std::invalid_argument(
        "Unsupported resource type: " + std::string{value});
}

}  // namespace haven::domain
