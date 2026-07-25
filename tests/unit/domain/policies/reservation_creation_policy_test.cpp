/**
 * @file reservation_creation_policy_test.cpp
 * @brief Tests reservation creation decision evaluation.
 */

#include "haven/domain/policies/reservation_creation_policy.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

Resource create_resource(const ResourceStatus status, const bool requires_approval) {
    return Resource{
        OrganizationId{"organization-123"},
        ResourceId{"resource-123"},
        ResourceType::MeetingRoom,
        status,
        requires_approval};
}

TimeInterval create_interval(const std::chrono::hours duration) {
    const TimeInterval::TimePoint start{};

    return TimeInterval{start, start + duration};
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldReturnConfirmed_WhenActiveResourceDoesNotRequireApproval) {
    const Resource resource = create_resource(ResourceStatus::Active, false);
    const ReservationCreationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(resource, create_interval(2h), ReservationKind::Standard, false),
        ReservationCreationDecision::Confirmed);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldReturnPendingApproval_WhenActiveResourceRequiresApproval) {
    const Resource resource = create_resource(ResourceStatus::Active, true);
    const ReservationCreationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(resource, create_interval(2h), ReservationKind::Standard, false),
        ReservationCreationDecision::PendingApproval);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldRejectCreation_WhenResourceIsInactive) {
    const Resource resource = create_resource(ResourceStatus::Inactive, false);
    const ReservationCreationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(resource, create_interval(2h), ReservationKind::Standard, false),
        ReservationCreationDecision::ResourceInactive);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldRejectCreation_WhenStandardDurationExceedsLimit) {
    const Resource resource = create_resource(ResourceStatus::Active, false);
    const ReservationCreationPolicy policy;
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 12h + 1min};

    EXPECT_EQ(
        policy.evaluate(resource, interval, ReservationKind::Standard, false),
        ReservationCreationDecision::StandardDurationExceeded);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldRejectCreation_WhenMaintenanceIsUnauthorized) {
    const Resource resource = create_resource(ResourceStatus::Active, false);
    const ReservationCreationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(resource, create_interval(2h), ReservationKind::Maintenance, false),
        ReservationCreationDecision::MaintenanceAuthorizationRequired);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldReturnConfirmed_WhenMaintenanceIsAuthorized) {
    const Resource resource = create_resource(ResourceStatus::Active, false);
    const ReservationCreationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(resource, create_interval(24h), ReservationKind::Maintenance, true),
        ReservationCreationDecision::Confirmed);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldReturnPendingApproval_WhenAuthorizedMaintenanceRequiresApproval) {
    const Resource resource = create_resource(ResourceStatus::Active, true);
    const ReservationCreationPolicy policy;

    EXPECT_EQ(
        policy.evaluate(resource, create_interval(24h), ReservationKind::Maintenance, true),
        ReservationCreationDecision::PendingApproval);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldRejectCreation_WhenMaintenanceDurationExceedsLimit) {
    const Resource resource = create_resource(ResourceStatus::Active, false);
    const ReservationCreationPolicy policy;
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 24h + 1min};

    EXPECT_EQ(
        policy.evaluate(resource, interval, ReservationKind::Maintenance, true),
        ReservationCreationDecision::MaintenanceDurationExceeded);
}

TEST(ReservationCreationPolicyTest, Evaluate_ShouldCheckResourceActivityBeforeReservationPolicy) {
    const Resource resource = create_resource(ResourceStatus::Inactive, false);
    const ReservationCreationPolicy policy;
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 25h};

    EXPECT_EQ(
        policy.evaluate(resource, interval, ReservationKind::Maintenance, false),
        ReservationCreationDecision::ResourceInactive);
}

}  // namespace
}  // namespace haven::domain