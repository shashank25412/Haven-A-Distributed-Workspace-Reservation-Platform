/**
 * @file resource_status_test.cpp
 * @brief Tests the supported Haven resource states.
 */

#include "haven/domain/value_objects/resource_status.hpp"

#include <gtest/gtest.h>

namespace haven::domain {
namespace {

TEST(ResourceStatusTest, ToString_ShouldReturnActive_WhenStatusIsActive) {
    EXPECT_EQ(to_string(ResourceStatus::Active), "ACTIVE");
}

TEST(ResourceStatusTest, ToString_ShouldReturnInactive_WhenStatusIsInactive) {
    EXPECT_EQ(to_string(ResourceStatus::Inactive), "INACTIVE");
}

TEST(ResourceStatusTest, ToString_ShouldReturnUnknown_WhenStatusIsUnsupported) {
    const auto unsupported_status = static_cast<ResourceStatus>(999);

    EXPECT_EQ(to_string(unsupported_status), "UNKNOWN");
}

TEST(ResourceStatusTest, FromString_ShouldReturnSupportedResourceStatus) {
    EXPECT_EQ(resource_status_from_string("INACTIVE"), ResourceStatus::Inactive);
}

TEST(ResourceStatusTest, FromString_ShouldRejectUnsupportedResourceStatus) {
    EXPECT_THROW(
        static_cast<void>(resource_status_from_string("ARCHIVED")),
        std::invalid_argument);
}

}  // namespace
}  // namespace haven::domain
