/**
 * @file organization_test.cpp
 * @brief Tests the organization directory aggregate.
 */

#include "haven/domain/organization.hpp"

#include "haven/domain/value_objects/organization_id.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::domain {
namespace {

TEST(OrganizationTest, Constructor_ShouldStoreFields_WhenNameIsValid) {
    const Organization organization{OrganizationId{"organization-1"}, "Organization One"};

    EXPECT_EQ(organization.organization_id(), OrganizationId{"organization-1"});
    EXPECT_EQ(organization.name(), "Organization One");
}

TEST(OrganizationTest, Constructor_ShouldThrow_WhenNameIsEmpty) {
    EXPECT_THROW(Organization(OrganizationId{"organization-1"}, ""), std::invalid_argument);
}

TEST(OrganizationTest, Equality_ShouldReturnTrue_WhenFieldsMatch) {
    const Organization first{OrganizationId{"organization-1"}, "Organization One"};
    const Organization second{OrganizationId{"organization-1"}, "Organization One"};

    EXPECT_EQ(first, second);
}

TEST(OrganizationTest, Equality_ShouldReturnFalse_WhenNameDiffers) {
    const Organization first{OrganizationId{"organization-1"}, "Organization One"};
    const Organization second{OrganizationId{"organization-1"}, "Organization Two"};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain
