/**
 * @file event_id_test.cpp
 * @brief Tests the event identifier domain value object.
 */

#include "haven/domain/value_objects/event_id.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(EventIdTest, Constructor_ShouldStoreValue_WhenIdentifierIsValid) {
    const EventId event_id{"event-123"};

    EXPECT_EQ(event_id.value(), "event-123");
}

TEST(EventIdTest, Constructor_ShouldThrow_WhenIdentifierIsEmpty) {
    EXPECT_THROW(EventId{""}, std::invalid_argument);
}

TEST(EventIdTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const EventId first{"event-123"};
    const EventId second{"event-123"};

    EXPECT_EQ(first, second);
}

TEST(EventIdTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const EventId first{"event-123"};
    const EventId second{"event-456"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain