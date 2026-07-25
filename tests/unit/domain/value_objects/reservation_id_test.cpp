/**
 * @file reservation_id_test.cpp
 * @brief Tests the reservation identifier domain value object.
 */

#include "haven/domain/value_objects/reservation_id.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(ReservationIdTest, Constructor_ShouldStoreValue_WhenIdentifierIsValid) {
    const ReservationId reservation_id{"reservation-123"};

    EXPECT_EQ(reservation_id.value(), "reservation-123");
}

TEST(ReservationIdTest, Constructor_ShouldThrow_WhenIdentifierIsEmpty) {
    EXPECT_THROW(ReservationId{""}, std::invalid_argument);
}

TEST(ReservationIdTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationId first{"reservation-123"};
    const ReservationId second{"reservation-123"};

    EXPECT_EQ(first, second);
}

TEST(ReservationIdTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const ReservationId first{"reservation-123"};
    const ReservationId second{"reservation-456"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain