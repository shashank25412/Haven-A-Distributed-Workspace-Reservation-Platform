/**
 * @file reservation_conflict_policy_test.cpp
 * @brief Tests reservation conflict evaluation.
 */

#include "haven/domain/policies/reservation_conflict_policy.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

TEST(ReservationConflictPolicyTest, BlocksSchedule_ShouldReturnTrue_WhenStatusIsConfirmed) {
    EXPECT_TRUE(blocks_schedule(ReservationStatus::Confirmed));
}

TEST(ReservationConflictPolicyTest, BlocksSchedule_ShouldReturnFalse_WhenStatusIsPendingApproval) {
    EXPECT_FALSE(blocks_schedule(ReservationStatus::PendingApproval));
}

class NonBlockingReservationStatusTest : public ::testing::TestWithParam<ReservationStatus> {
};

TEST_P(NonBlockingReservationStatusTest, BlocksSchedule_ShouldReturnFalse_WhenStatusDoesNotClaimSchedule) {
    EXPECT_FALSE(blocks_schedule(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    TerminalStatuses,
    NonBlockingReservationStatusTest,
    ::testing::Values(
        ReservationStatus::Cancelled,
        ReservationStatus::Rejected,
        ReservationStatus::Expired,
        ReservationStatus::Completed));

TEST(ReservationConflictPolicyTest, HasReservationConflict_ShouldReturnTrue_WhenConfirmedIntervalsPartiallyOverlap) {
    const TimeInterval::TimePoint start{};
    const TimeInterval requested_interval{start, start + 2h};
    const TimeInterval existing_interval{start + 1h, start + 3h};

    EXPECT_TRUE(has_reservation_conflict(
        requested_interval,
        existing_interval,
        ReservationStatus::Confirmed));
}

TEST(ReservationConflictPolicyTest, HasReservationConflict_ShouldReturnTrue_WhenConfirmedIntervalContainsRequest) {
    const TimeInterval::TimePoint start{};
    const TimeInterval requested_interval{start + 1h, start + 2h};
    const TimeInterval existing_interval{start, start + 3h};

    EXPECT_TRUE(has_reservation_conflict(
        requested_interval,
        existing_interval,
        ReservationStatus::Confirmed));
}

TEST(ReservationConflictPolicyTest, HasReservationConflict_ShouldReturnFalse_WhenConfirmedIntervalsAreAdjacent) {
    const TimeInterval::TimePoint start{};
    const TimeInterval requested_interval{start + 1h, start + 2h};
    const TimeInterval existing_interval{start, start + 1h};

    EXPECT_FALSE(has_reservation_conflict(
        requested_interval,
        existing_interval,
        ReservationStatus::Confirmed));
}

TEST(ReservationConflictPolicyTest, HasReservationConflict_ShouldReturnFalse_WhenConfirmedIntervalsAreSeparated) {
    const TimeInterval::TimePoint start{};
    const TimeInterval requested_interval{start + 2h, start + 3h};
    const TimeInterval existing_interval{start, start + 1h};

    EXPECT_FALSE(has_reservation_conflict(
        requested_interval,
        existing_interval,
        ReservationStatus::Confirmed));
}

TEST(ReservationConflictPolicyTest, HasReservationConflict_ShouldReturnFalse_WhenPendingIntervalsOverlap) {
    const TimeInterval::TimePoint start{};
    const TimeInterval requested_interval{start, start + 2h};
    const TimeInterval existing_interval{start + 1h, start + 3h};

    EXPECT_FALSE(has_reservation_conflict(
        requested_interval,
        existing_interval,
        ReservationStatus::PendingApproval));
}

TEST_P(NonBlockingReservationStatusTest, HasReservationConflict_ShouldReturnFalse_WhenTerminalIntervalOverlaps) {
    const TimeInterval::TimePoint start{};
    const TimeInterval requested_interval{start, start + 2h};
    const TimeInterval existing_interval{start + 1h, start + 3h};

    EXPECT_FALSE(has_reservation_conflict(
        requested_interval,
        existing_interval,
        GetParam()));
}

}  // namespace
}  // namespace haven::domain