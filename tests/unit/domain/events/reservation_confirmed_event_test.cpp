/**
 * @file reservation_confirmed_event_test.cpp
 * @brief Tests the reservation-confirmed domain event.
 */

#include "haven/domain/events/reservation_confirmed_event.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationConfirmedEvent create_confirmed_event(std::optional<UserId> confirmed_by) {
    const ReservationConfirmedEvent::TimePoint occurred_at = ReservationConfirmedEvent::TimePoint{} + 30min;
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 1h;

    return ReservationConfirmedEvent{
        EventId{"event-123"},
        occurred_at,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        TimeInterval{start, start + 2h},
        std::move(confirmed_by)};
}

TEST(ReservationConfirmedEventTest, Constructor_ShouldStoreIdentity_WhenReservationIsConfirmed) {
    const ReservationConfirmedEvent event = create_confirmed_event(std::nullopt);

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
}

TEST(ReservationConfirmedEventTest, Constructor_ShouldStoreInterval_WhenReservationIsConfirmed) {
    const ReservationConfirmedEvent event = create_confirmed_event(std::nullopt);

    EXPECT_EQ(event.interval().duration(), 2h);
}

TEST(ReservationConfirmedEventTest, Constructor_ShouldAllowMissingUser_WhenReservationIsAutomaticallyConfirmed) {
    const ReservationConfirmedEvent event = create_confirmed_event(std::nullopt);

    EXPECT_FALSE(event.confirmed_by().has_value());
}

TEST(ReservationConfirmedEventTest, Constructor_ShouldStoreUser_WhenPendingReservationIsApproved) {
    const ReservationConfirmedEvent event = create_confirmed_event(UserId{"approver-123"});

    ASSERT_TRUE(event.confirmed_by().has_value());
    EXPECT_EQ(event.confirmed_by().value(), UserId{"approver-123"});
}

TEST(ReservationConfirmedEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationConfirmedEvent event = create_confirmed_event(std::nullopt);

    EXPECT_EQ(event.occurred_at(), ReservationConfirmedEvent::TimePoint{} + 30min);
}

TEST(ReservationConfirmedEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationConfirmedEvent first = create_confirmed_event(UserId{"approver-123"});
    const ReservationConfirmedEvent second = create_confirmed_event(UserId{"approver-123"});

    EXPECT_EQ(first, second);
}

TEST(ReservationConfirmedEventTest, Equality_ShouldReturnFalse_WhenConfirmingUsersDiffer) {
    const ReservationConfirmedEvent first = create_confirmed_event(UserId{"approver-123"});
    const ReservationConfirmedEvent second = create_confirmed_event(UserId{"approver-456"});

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain