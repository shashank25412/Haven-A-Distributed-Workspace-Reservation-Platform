/**
 * @file version_test.cpp
 * @brief Tests the optimistic concurrency version domain value object.
 */

#include "haven/domain/value_objects/version.hpp"

#include <gtest/gtest.h>

namespace haven::domain {
namespace {

TEST(VersionTest, Constructor_ShouldStoreValue_WhenVersionIsProvided) {
    const Version version{42};

    EXPECT_EQ(version.value(), 42U);
}

TEST(VersionTest, Constructor_ShouldAllowZero_WhenEntityHasNoPersistedVersion) {
    const Version version{0};

    EXPECT_EQ(version.value(), 0U);
}

TEST(VersionTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const Version first{42};
    const Version second{42};

    EXPECT_EQ(first, second);
}

TEST(VersionTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const Version first{42};
    const Version second{43};

    EXPECT_NE(first, second);
}

TEST(VersionTest, Comparison_ShouldOrderVersions_ByStoredValue) {
    const Version earlier{42};
    const Version later{43};

    EXPECT_LT(earlier, later);
}

}  // namespace
}  // namespace haven::domain