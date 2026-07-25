/**
 * @file user_id_test.cpp
 * @brief Tests the user identifier domain value object.
 */

#include "haven/domain/value_objects/user_id.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(UserIdTest, Constructor_ShouldStoreValue_WhenIdentifierIsValid) {
    const UserId user_id{"user-123"};

    EXPECT_EQ(user_id.value(), "user-123");
}

TEST(UserIdTest, Constructor_ShouldThrow_WhenIdentifierIsEmpty) {
    EXPECT_THROW(UserId{""}, std::invalid_argument);
}

TEST(UserIdTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const UserId first{"user-123"};
    const UserId second{"user-123"};

    EXPECT_EQ(first, second);
}

TEST(UserIdTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const UserId first{"user-123"};
    const UserId second{"user-456"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain