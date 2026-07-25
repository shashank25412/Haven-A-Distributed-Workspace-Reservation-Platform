/**
 * @file reservation_approval_requested_event_test.cpp
 * @brief Tests the reservation-approval-requested domain event.
 */

#include "haven/domain/events/reservation_approval_requested_event.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationApprovalRequestedEvent create_approval_requested_event() {
    const ReservationApprovalRequestedEvent::TimePoint occurred_at =
        ReservationApprovalRequestedEvent::TimePoint{} + 30min;
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    return ReservationApprovalRequestedEvent{
        EventId{"event-123"},
        occurred_at,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"user-123"},
        TimeInterval{start, start + 2h},
        ReservationKind::Standard};
}

TEST(ReservationApprovalRequestedEventTest, Constructor_ShouldStoreEventIdentity_WhenEventIsCreated) {
    const ReservationApprovalRequestedEvent event = create_approval_requested_event();

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
}

TEST(ReservationApprovalRequestedEventTest, Constructor_ShouldStoreReservationContext_WhenEventIsCreated) {
    const ReservationApprovalRequestedEvent event = create_approval_requested_event();

    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(event.requested_by(), UserId{"user-123"});
    EXPECT_EQ(event.interval().duration(), 2h);
    EXPECT_EQ(event.kind(), ReservationKind::Standard);
}

TEST(ReservationApprovalRequestedEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationApprovalRequestedEvent event = create_approval_requested_event();

    EXPECT_EQ(event.occurred_at(), ReservationApprovalRequestedEvent::TimePoint{} + 30min);
}

TEST(ReservationApprovalRequestedEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationApprovalRequestedEvent first = create_approval_requested_event();
    const ReservationApprovalRequestedEvent second = create_approval_requested_event();

    EXPECT_EQ(first, second);
}

TEST(ReservationApprovalRequestedEventTest, Equality_ShouldReturnFalse_WhenEventIdentifiersDiffer) {
    const ReservationApprovalRequestedEvent first = create_approval_requested_event();
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;
    const ReservationApprovalRequestedEvent second{
        EventId{"event-456"},
        ReservationApprovalRequestedEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"user-123"},
        TimeInterval{start, start + 2h},
        ReservationKind::Standard};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain