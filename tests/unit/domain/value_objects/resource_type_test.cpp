/**
 * @file resource_type_test.cpp
 * @brief Tests the supported Haven resource types.
 */

#include "haven/domain/value_objects/resource_type.hpp"

#include <gtest/gtest.h>

namespace haven::domain {
namespace {

TEST(ResourceTypeTest, ToString_ShouldReturnMeetingRoom_WhenTypeIsMeetingRoom) {
    EXPECT_EQ(to_string(ResourceType::MeetingRoom), "MEETING_ROOM");
}

TEST(ResourceTypeTest, ToString_ShouldReturnOfficeDesk_WhenTypeIsOfficeDesk) {
    EXPECT_EQ(to_string(ResourceType::OfficeDesk), "OFFICE_DESK");
}

TEST(ResourceTypeTest, ToString_ShouldReturnParkingSlot_WhenTypeIsParkingSlot) {
    EXPECT_EQ(to_string(ResourceType::ParkingSlot), "PARKING_SLOT");
}

TEST(ResourceTypeTest, ToString_ShouldReturnHotelRoom_WhenTypeIsHotelRoom) {
    EXPECT_EQ(to_string(ResourceType::HotelRoom), "HOTEL_ROOM");
}

TEST(ResourceTypeTest, ToString_ShouldReturnGameZone_WhenTypeIsGameZone) {
    EXPECT_EQ(to_string(ResourceType::GameZone), "GAME_ZONE");
}

TEST(ResourceTypeTest, ToString_ShouldReturnUnknown_WhenTypeIsUnsupported) {
    const auto unsupported_type = static_cast<ResourceType>(999);

    EXPECT_EQ(to_string(unsupported_type), "UNKNOWN");
}

}  // namespace
}  // namespace haven::domain