/**
 * @file reservation_policy_test.cpp
 * @brief Tests reservation creation policy evaluation.
 */

#include "haven/domain/policies/reservation_policy.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

TEST(ReservationPolicyTest, Evaluate_ShouldAllowStandardReservation_WhenDurationIsBelowLimit) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 2h};
    const ReservationPolicy policy;

    EXPECT_EQ(policy.evaluate(interval, ReservationKind::Standard, false), ReservationPolicyViolation::None);
}

TEST(ReservationPolicyTest, Evaluate_ShouldAllowStandardReservation_WhenDurationEqualsLimit) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 12h};
    const ReservationPolicy policy;

    EXPECT_EQ(policy.evaluate(interval, ReservationKind::Standard, false), ReservationPolicyViolation::None);
}

TEST(ReservationPolicyTest, Evaluate_ShouldRejectStandardReservation_WhenDurationExceedsLimit) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 12h + 1min};
    const ReservationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(interval, ReservationKind::Standard, false),
        ReservationPolicyViolation::StandardDurationExceeded);
}

TEST(ReservationPolicyTest, Evaluate_ShouldRejectMaintenanceReservation_WhenCallerIsUnauthorized) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 2h};
    const ReservationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(interval, ReservationKind::Maintenance, false),
        ReservationPolicyViolation::MaintenanceAuthorizationRequired);
}

TEST(ReservationPolicyTest, Evaluate_ShouldAllowMaintenanceReservation_WhenDurationEqualsLimitAndCallerIsAuthorized) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 24h};
    const ReservationPolicy policy;

    EXPECT_EQ(policy.evaluate(interval, ReservationKind::Maintenance, true), ReservationPolicyViolation::None);
}

TEST(ReservationPolicyTest, Evaluate_ShouldRejectMaintenanceReservation_WhenDurationExceedsLimit) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 24h + 1min};
    const ReservationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(interval, ReservationKind::Maintenance, true),
        ReservationPolicyViolation::MaintenanceDurationExceeded);
}

TEST(ReservationPolicyTest, Evaluate_ShouldCheckAuthorizationBeforeDuration_WhenMaintenanceRequestIsUnauthorized) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 25h};
    const ReservationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(interval, ReservationKind::Maintenance, false),
        ReservationPolicyViolation::MaintenanceAuthorizationRequired);
}

}  // namespace
}  // namespace haven::domain