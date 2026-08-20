/**
 * @file rejection_info_test.cpp
 * @brief Tests the reservation rejection information domain value object.
 */

#include "haven/domain/value_objects/rejection_info.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

TEST(RejectionInfoTest, Constructor_ShouldStoreRejector_WhenRejectionIsCreated) {
    const RejectionInfo rejection_info{UserId{"approver-123"}, RejectionInfo::TimePoint{}};

    EXPECT_EQ(rejection_info.rejected_by(), UserId{"approver-123"});
}

TEST(RejectionInfoTest, Constructor_ShouldStoreRejectionTime_WhenRejectionIsCreated) {
    const RejectionInfo::TimePoint rejected_at = RejectionInfo::TimePoint{} + 10h;
    const RejectionInfo rejection_info{UserId{"approver-123"}, rejected_at};

    EXPECT_EQ(rejection_info.rejected_at(), rejected_at);
}

TEST(RejectionInfoTest, Constructor_ShouldDefaultReasonToEmpty_WhenReasonIsOmitted) {
    const RejectionInfo rejection_info{UserId{"approver-123"}, RejectionInfo::TimePoint{}};

    EXPECT_FALSE(rejection_info.reason().has_value());
}

TEST(RejectionInfoTest, Constructor_ShouldStoreReason_WhenReasonIsProvided) {
    const RejectionInfo rejection_info{
        UserId{"approver-123"}, RejectionInfo::TimePoint{}, std::string{"Conflicts with maintenance."}};

    ASSERT_TRUE(rejection_info.reason().has_value());
    EXPECT_EQ(*rejection_info.reason(), "Conflicts with maintenance.");
}

TEST(RejectionInfoTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const RejectionInfo::TimePoint rejected_at = RejectionInfo::TimePoint{} + 10h;
    const RejectionInfo first{UserId{"approver-123"}, rejected_at, std::string{"Reason"}};
    const RejectionInfo second{UserId{"approver-123"}, rejected_at, std::string{"Reason"}};

    EXPECT_EQ(first, second);
}

TEST(RejectionInfoTest, Equality_ShouldReturnFalse_WhenReasonsDiffer) {
    const RejectionInfo::TimePoint rejected_at = RejectionInfo::TimePoint{} + 10h;
    const RejectionInfo first{UserId{"approver-123"}, rejected_at, std::string{"Reason"}};
    const RejectionInfo second{UserId{"approver-123"}, rejected_at, std::string{"Other reason"}};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain
