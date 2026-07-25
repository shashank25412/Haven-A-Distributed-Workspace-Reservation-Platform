/**
 * @file resource_type.cpp
 * @brief Implements conversion for supported Haven resource types.
 */

#include "haven/domain/value_objects/resource_type.hpp"

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

}  // namespace haven::domain