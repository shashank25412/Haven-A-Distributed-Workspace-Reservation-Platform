/**
 * @file approval_info_test.cpp
 * @brief Tests the reservation approval information domain value object.
 */

#include "haven/domain/value_objects/approval_info.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

TEST(ApprovalInfoTest, Constructor_ShouldStoreApprover_WhenApprovalIsCreated) {
    const ApprovalInfo approval_info{UserId{"approver-123"}, ApprovalInfo::TimePoint{}};

    EXPECT_EQ(approval_info.approved_by(), UserId{"approver-123"});
}

TEST(ApprovalInfoTest, Constructor_ShouldStoreApprovalTime_WhenApprovalIsCreated) {
    const ApprovalInfo::TimePoint approved_at = ApprovalInfo::TimePoint{} + 10h;
    const ApprovalInfo approval_info{UserId{"approver-123"}, approved_at};

    EXPECT_EQ(approval_info.approved_at(), approved_at);
}

TEST(ApprovalInfoTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ApprovalInfo::TimePoint approved_at = ApprovalInfo::TimePoint{} + 10h;
    const ApprovalInfo first{UserId{"approver-123"}, approved_at};
    const ApprovalInfo second{UserId{"approver-123"}, approved_at};

    EXPECT_EQ(first, second);
}

TEST(ApprovalInfoTest, Equality_ShouldReturnFalse_WhenApproversDiffer) {
    const ApprovalInfo::TimePoint approved_at = ApprovalInfo::TimePoint{} + 10h;
    const ApprovalInfo first{UserId{"approver-123"}, approved_at};
    const ApprovalInfo second{UserId{"approver-456"}, approved_at};

    EXPECT_NE(first, second);
}

TEST(ApprovalInfoTest, Equality_ShouldReturnFalse_WhenApprovalTimesDiffer) {
    const ApprovalInfo::TimePoint approved_at = ApprovalInfo::TimePoint{} + 10h;
    const ApprovalInfo first{UserId{"approver-123"}, approved_at};
    const ApprovalInfo second{UserId{"approver-123"}, approved_at + 1min};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain