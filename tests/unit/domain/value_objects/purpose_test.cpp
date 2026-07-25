/**
 * @file purpose_test.cpp
 * @brief Tests the reservation purpose domain value object.
 */

#include "haven/domain/value_objects/purpose.hpp"

#include <gtest/gtest.h>

namespace haven::domain {
namespace {

TEST(PurposeTest, Constructor_ShouldStoreValue_WhenPurposeContainsText) {
    const Purpose purpose{"Quarterly planning meeting"};

    EXPECT_EQ(purpose.value(), "Quarterly planning meeting");
}

TEST(PurposeTest, Constructor_ShouldAllowEmptyValue_WhenPurposeIsNotProvided) {
    const Purpose purpose{""};

    EXPECT_TRUE(purpose.value().empty());
}

TEST(PurposeTest, Constructor_ShouldPreserveWhitespace_WhenPurposeContainsOnlyWhitespace) {
    const Purpose purpose{" \t\n "};

    EXPECT_EQ(purpose.value(), " \t\n ");
}

TEST(PurposeTest, Constructor_ShouldPreserveSurroundingWhitespace_WhenPurposeContainsText) {
    const Purpose purpose{"  Team workshop  "};

    EXPECT_EQ(purpose.value(), "  Team workshop  ");
}

TEST(PurposeTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const Purpose first{"Team workshop"};
    const Purpose second{"Team workshop"};

    EXPECT_EQ(first, second);
}

TEST(PurposeTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const Purpose first{"Team workshop"};
    const Purpose second{"Client meeting"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain