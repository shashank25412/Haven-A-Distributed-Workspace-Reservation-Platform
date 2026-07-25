/**
 * @file reservation_cancelled_event_test.cpp
 * @brief Tests the reservation-cancelled domain event.
 */

#include "haven/domain/events/reservation_cancelled_event.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationCancelledEvent create_cancelled_event(const ReservationStatus previous_status) {
    return ReservationCancelledEvent{
        EventId{"event-123"},
        ReservationCancelledEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"user-123"},
        previous_status};
}

TEST(ReservationCancelledEventTest, Constructor_ShouldStoreIdentity_WhenConfirmedReservationIsCancelled) {
    const ReservationCancelledEvent event = create_cancelled_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
}

TEST(ReservationCancelledEventTest, Constructor_ShouldStoreActor_WhenReservationIsCancelled) {
    const ReservationCancelledEvent event = create_cancelled_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.cancelled_by(), UserId{"user-123"});
}

TEST(ReservationCancelledEventTest, Constructor_ShouldStorePreviousStatus_WhenConfirmedReservationIsCancelled) {
    const ReservationCancelledEvent event = create_cancelled_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.previous_status(), ReservationStatus::Confirmed);
}

TEST(ReservationCancelledEventTest, Constructor_ShouldAllowPendingApprovalAsPreviousStatus) {
    const ReservationCancelledEvent event = create_cancelled_event(ReservationStatus::PendingApproval);

    EXPECT_EQ(event.previous_status(), ReservationStatus::PendingApproval);
}

TEST(ReservationCancelledEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationCancelledEvent event = create_cancelled_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.occurred_at(), ReservationCancelledEvent::TimePoint{} + 30min);
}

TEST(ReservationCancelledEventTest, Constructor_ShouldThrow_WhenPreviousStatusIsTerminal) {
    EXPECT_THROW(create_cancelled_event(ReservationStatus::Rejected), std::invalid_argument);
}

TEST(ReservationCancelledEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationCancelledEvent first = create_cancelled_event(ReservationStatus::Confirmed);
    const ReservationCancelledEvent second = create_cancelled_event(ReservationStatus::Confirmed);

    EXPECT_EQ(first, second);
}

TEST(ReservationCancelledEventTest, Equality_ShouldReturnFalse_WhenPreviousStatusesDiffer) {
    const ReservationCancelledEvent first = create_cancelled_event(ReservationStatus::Confirmed);
    const ReservationCancelledEvent second = create_cancelled_event(ReservationStatus::PendingApproval);

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain