/**
 * @file reservation_created_event_test.cpp
 * @brief Tests the reservation-created domain event.
 */

#include "haven/domain/events/reservation_created_event.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationCreatedEvent create_reservation_created_event(const ReservationStatus initial_status) {
    const ReservationCreatedEvent::TimePoint occurred_at = ReservationCreatedEvent::TimePoint{} + 30min;
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    return ReservationCreatedEvent{
        EventId{"event-123"},
        occurred_at,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        UserId{"user-123"},
        TimeInterval{start, start + 2h},
        ReservationKind::Standard,
        initial_status};
}

TEST(ReservationCreatedEventTest, Constructor_ShouldStoreEventIdentity_WhenInitialStatusIsConfirmed) {
    const ReservationCreatedEvent event = create_reservation_created_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
}

TEST(ReservationCreatedEventTest, Constructor_ShouldStoreReservationDetails_WhenInitialStatusIsConfirmed) {
    const ReservationCreatedEvent event = create_reservation_created_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.created_by(), UserId{"user-123"});
    EXPECT_EQ(event.kind(), ReservationKind::Standard);
    EXPECT_EQ(event.initial_status(), ReservationStatus::Confirmed);
    EXPECT_EQ(event.interval().duration(), 2h);
}

TEST(ReservationCreatedEventTest, Constructor_ShouldAllowPendingApproval_WhenReservationRequiresApproval) {
    const ReservationCreatedEvent event = create_reservation_created_event(ReservationStatus::PendingApproval);

    EXPECT_EQ(event.initial_status(), ReservationStatus::PendingApproval);
}

TEST(ReservationCreatedEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationCreatedEvent event = create_reservation_created_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.occurred_at(), ReservationCreatedEvent::TimePoint{} + 30min);
}

TEST(ReservationCreatedEventTest, Constructor_ShouldThrow_WhenInitialStatusIsTerminal) {
    EXPECT_THROW(create_reservation_created_event(ReservationStatus::Cancelled), std::invalid_argument);
}

}  // namespace
}  // namespace haven::domain