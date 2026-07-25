/**
 * @file reservation_completed_event_test.cpp
 * @brief Tests the reservation-completed domain event.
 */

#include "haven/domain/events/reservation_completed_event.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationCompletedEvent create_completed_event() {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    return ReservationCompletedEvent{
        EventId{"event-123"},
        ReservationCompletedEvent::TimePoint{} + 3h,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        TimeInterval{start, start + 2h}};
}

TEST(ReservationCompletedEventTest, Constructor_ShouldStoreIdentity_WhenReservationIsCompleted) {
    const ReservationCompletedEvent event = create_completed_event();

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
}

TEST(ReservationCompletedEventTest, Constructor_ShouldStoreInterval_WhenReservationIsCompleted) {
    const ReservationCompletedEvent event = create_completed_event();

    EXPECT_EQ(event.interval().duration(), 2h);
}

TEST(ReservationCompletedEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationCompletedEvent event = create_completed_event();

    EXPECT_EQ(event.occurred_at(), ReservationCompletedEvent::TimePoint{} + 3h);
}

TEST(ReservationCompletedEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationCompletedEvent first = create_completed_event();
    const ReservationCompletedEvent second = create_completed_event();

    EXPECT_EQ(first, second);
}

TEST(ReservationCompletedEventTest, Equality_ShouldReturnFalse_WhenEventIdentifiersDiffer) {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;
    const ReservationCompletedEvent first = create_completed_event();
    const ReservationCompletedEvent second{
        EventId{"event-456"},
        ReservationCompletedEvent::TimePoint{} + 3h,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        TimeInterval{start, start + 2h}};

    EXPECT_NE(first, second);
}

TEST(ReservationCompletedEventTest, Equality_ShouldReturnFalse_WhenIntervalsDiffer) {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;
    const ReservationCompletedEvent first = create_completed_event();
    const ReservationCompletedEvent second{
        EventId{"event-123"},
        ReservationCompletedEvent::TimePoint{} + 3h,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        TimeInterval{start, start + 3h}};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain