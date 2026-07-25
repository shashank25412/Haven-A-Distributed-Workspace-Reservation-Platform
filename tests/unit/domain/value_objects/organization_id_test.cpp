/**
 * @file organization_id_test.cpp
 * @brief Tests the organization identifier domain value object.
 */

#include "haven/domain/value_objects/organization_id.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace haven::domain {
namespace {

TEST(OrganizationIdTest, Constructor_ShouldStoreValue_WhenIdentifierIsValid) {
    const OrganizationId organization_id{"organization-123"};

    EXPECT_EQ(organization_id.value(), "organization-123");
}

TEST(OrganizationIdTest, Constructor_ShouldThrow_WhenIdentifierIsEmpty) {
    EXPECT_THROW(OrganizationId{""}, std::invalid_argument);
}

TEST(OrganizationIdTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const OrganizationId first{"organization-123"};
    const OrganizationId second{"organization-123"};

    EXPECT_EQ(first, second);
}

TEST(OrganizationIdTest, Equality_ShouldReturnFalse_WhenValuesDiffer) {
    const OrganizationId first{"organization-123"};
    const OrganizationId second{"organization-456"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain