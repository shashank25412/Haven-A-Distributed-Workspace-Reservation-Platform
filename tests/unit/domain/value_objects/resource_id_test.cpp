/**
 * @file resource_id_test.cpp
 * @brief Tests the resource identifier domain value object.
 */

#include "haven/domain/value_objects/resource_id.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(ResourceIdTest, Constructor_ShouldStoreValue_WhenIdentifierIsValid) {
    const ResourceId resource_id{"resource-123"};

    EXPECT_EQ(resource_id.value(), "resource-123");
}

TEST(ResourceIdTest, Constructor_ShouldThrow_WhenIdentifierIsEmpty) {
    EXPECT_THROW(ResourceId{""}, std::invalid_argument);
}

TEST(ResourceIdTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ResourceId first{"resource-123"};
    const ResourceId second{"resource-123"};

    EXPECT_EQ(first, second);
}

TEST(ResourceIdTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const ResourceId first{"resource-123"};
    const ResourceId second{"resource-456"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain