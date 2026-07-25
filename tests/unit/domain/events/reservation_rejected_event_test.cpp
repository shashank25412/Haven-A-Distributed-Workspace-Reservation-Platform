/**
 * @file reservation_rejected_event_test.cpp
 * @brief Tests the reservation-rejected domain event.
 */

#include "haven/domain/events/reservation_rejected_event.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationRejectedEvent create_rejected_event() {
    return ReservationRejectedEvent{
        EventId{"event-123"},
        ReservationRejectedEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"approver-123"}};
}

TEST(ReservationRejectedEventTest, Constructor_ShouldStoreEventIdentity_WhenReservationIsRejected) {
    const ReservationRejectedEvent event = create_rejected_event();

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
}

TEST(ReservationRejectedEventTest, Constructor_ShouldStoreReservationContext_WhenReservationIsRejected) {
    const ReservationRejectedEvent event = create_rejected_event();

    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(event.rejected_by(), UserId{"approver-123"});
}

TEST(ReservationRejectedEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationRejectedEvent event = create_rejected_event();

    EXPECT_EQ(event.occurred_at(), ReservationRejectedEvent::TimePoint{} + 30min);
}

TEST(ReservationRejectedEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationRejectedEvent first = create_rejected_event();
    const ReservationRejectedEvent second = create_rejected_event();

    EXPECT_EQ(first, second);
}

TEST(ReservationRejectedEventTest, Equality_ShouldReturnFalse_WhenRejectingUsersDiffer) {
    const ReservationRejectedEvent first = create_rejected_event();
    const ReservationRejectedEvent second{
        EventId{"event-123"},
        ReservationRejectedEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"approver-456"}};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain