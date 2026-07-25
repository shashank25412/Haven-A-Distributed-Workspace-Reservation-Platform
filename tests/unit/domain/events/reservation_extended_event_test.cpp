/**
 * @file reservation_extended_event_test.cpp
 * @brief Tests the reservation-extended domain event.
 */

#include "haven/domain/events/reservation_extended_event.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationExtendedEvent create_extended_event() {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    return ReservationExtendedEvent{
        EventId{"event-123"},
        ReservationExtendedEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"user-123"},
        TimeInterval{start, start + 2h},
        TimeInterval{start, start + 3h}};
}

TEST(ReservationExtendedEventTest, Constructor_ShouldStoreIdentity_WhenReservationIsExtended) {
    const ReservationExtendedEvent event = create_extended_event();

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
}

TEST(ReservationExtendedEventTest, Constructor_ShouldStoreActor_WhenReservationIsExtended) {
    const ReservationExtendedEvent event = create_extended_event();

    EXPECT_EQ(event.extended_by(), UserId{"user-123"});
}

TEST(ReservationExtendedEventTest, Constructor_ShouldStorePreviousAndExtendedIntervals_WhenReservationIsExtended) {
    const ReservationExtendedEvent event = create_extended_event();

    EXPECT_EQ(event.previous_interval().duration(), 2h);
    EXPECT_EQ(event.extended_interval().duration(), 3h);
}

TEST(ReservationExtendedEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationExtendedEvent event = create_extended_event();

    EXPECT_EQ(event.occurred_at(), ReservationExtendedEvent::TimePoint{} + 30min);
}

TEST(ReservationExtendedEventTest, Constructor_ShouldThrow_WhenExtendedIntervalChangesStartTime) {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    EXPECT_THROW(
        ReservationExtendedEvent(
            EventId{"event-123"},
            ReservationExtendedEvent::TimePoint{} + 30min,
            OrganizationId{"organization-123"},
            ReservationId{"reservation-123"},
            ResourceId{"resource-123"},
            UserId{"user-123"},
            TimeInterval{start, start + 2h},
            TimeInterval{start + 30min, start + 3h}),
        std::invalid_argument);
}

TEST(ReservationExtendedEventTest, Constructor_ShouldThrow_WhenExtendedEndDoesNotMoveForward) {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    EXPECT_THROW(
        ReservationExtendedEvent(
            EventId{"event-123"},
            ReservationExtendedEvent::TimePoint{} + 30min,
            OrganizationId{"organization-123"},
            ReservationId{"reservation-123"},
            ResourceId{"resource-123"},
            UserId{"user-123"},
            TimeInterval{start, start + 2h},
            TimeInterval{start, start + 90min}),
        std::invalid_argument);
}

TEST(ReservationExtendedEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationExtendedEvent first = create_extended_event();
    const ReservationExtendedEvent second = create_extended_event();

    EXPECT_EQ(first, second);
}

TEST(ReservationExtendedEventTest, Equality_ShouldReturnFalse_WhenActorsDiffer) {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;
    const ReservationExtendedEvent first = create_extended_event();
    const ReservationExtendedEvent second{
        EventId{"event-123"},
        ReservationExtendedEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"user-456"},
        TimeInterval{start, start + 2h},
        TimeInterval{start, start + 3h}};

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain