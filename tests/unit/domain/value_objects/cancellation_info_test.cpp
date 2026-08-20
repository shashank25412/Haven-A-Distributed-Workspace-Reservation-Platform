/**
 * @file cancellation_info_test.cpp
 * @brief Tests the reservation cancellation information domain value object.
 */

#include "haven/domain/value_objects/cancellation_info.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

TEST(CancellationInfoTest, Constructor_ShouldStoreCanceller_WhenCancellationIsCreated) {
    const CancellationInfo cancellation_info{UserId{"admin-123"}, CancellationInfo::TimePoint{}};

    EXPECT_EQ(cancellation_info.cancelled_by(), UserId{"admin-123"});
}

TEST(CancellationInfoTest, Constructor_ShouldStoreCancellationTime_WhenCancellationIsCreated) {
    const CancellationInfo::TimePoint cancelled_at = CancellationInfo::TimePoint{} + 10h;
    const CancellationInfo cancellation_info{UserId{"admin-123"}, cancelled_at};

    EXPECT_EQ(cancellation_info.cancelled_at(), cancelled_at);
}

TEST(CancellationInfoTest, Constructor_ShouldDefaultReasonToEmpty_WhenReasonIsOmitted) {
    const CancellationInfo cancellation_info{UserId{"admin-123"}, CancellationInfo::TimePoint{}};

    EXPECT_FALSE(cancellation_info.reason().has_value());
}

TEST(CancellationInfoTest, Constructor_ShouldStoreReason_WhenReasonIsProvided) {
    const CancellationInfo cancellation_info{
        UserId{"admin-123"}, CancellationInfo::TimePoint{}, std::string{"Double-booked by mistake"}};

    ASSERT_TRUE(cancellation_info.reason().has_value());
    EXPECT_EQ(*cancellation_info.reason(), "Double-booked by mistake");
}

TEST(CancellationInfoTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const CancellationInfo::TimePoint cancelled_at = CancellationInfo::TimePoint{} + 10h;
    const CancellationInfo first{UserId{"admin-123"}, cancelled_at, std::string{"reason"}};
    const CancellationInfo second{UserId{"admin-123"}, cancelled_at, std::string{"reason"}};

    EXPECT_EQ(first, second);
}

TEST(CancellationInfoTest, Equality_ShouldReturnFalse_WhenReasonsDiffer) {
    const CancellationInfo::TimePoint cancelled_at = CancellationInfo::TimePoint{} + 10h;
    const CancellationInfo first{UserId{"admin-123"}, cancelled_at, std::string{"reason-a"}};
    const CancellationInfo second{UserId{"admin-123"}, cancelled_at, std::string{"reason-b"}};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain
