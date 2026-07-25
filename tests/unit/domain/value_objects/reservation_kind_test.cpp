/**
 * @file reservation_kind_test.cpp
 * @brief Tests the supported Haven reservation kinds.
 */

#include "haven/domain/value_objects/reservation_kind.hpp"

#include <gtest/gtest.h>

namespace haven::domain {
namespace {

TEST(ReservationKindTest, ToString_ShouldReturnStandard_WhenKindIsStandard) {
    EXPECT_EQ(to_string(ReservationKind::Standard), "STANDARD");
}

TEST(ReservationKindTest, ToString_ShouldReturnMaintenance_WhenKindIsMaintenance) {
    EXPECT_EQ(to_string(ReservationKind::Maintenance), "MAINTENANCE");
}

TEST(ReservationKindTest, ToString_ShouldReturnUnknown_WhenKindIsUnsupported) {
    const auto unsupported_kind = static_cast<ReservationKind>(999);

    EXPECT_EQ(to_string(unsupported_kind), "UNKNOWN");
}

}  // namespace
}  // namespace haven::domain