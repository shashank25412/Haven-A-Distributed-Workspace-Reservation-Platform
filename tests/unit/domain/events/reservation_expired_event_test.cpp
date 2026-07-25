/**
 * @file reservation_expired_event_test.cpp
 * @brief Tests the reservation-expired domain event.
 */

#include "haven/domain/events/reservation_expired_event.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

ReservationExpiredEvent create_expired_event(const ReservationStatus previous_status) {
    return ReservationExpiredEvent{
        EventId{"event-123"},
        ReservationExpiredEvent::TimePoint{} + 30min,
        OrganizationId{"organization-123"},
        ReservationId{"reservation-123"},
        ResourceId{"resource-123"},
        previous_status};
}

TEST(ReservationExpiredEventTest, Constructor_ShouldStoreIdentity_WhenConfirmedReservationExpires) {
    const ReservationExpiredEvent event = create_expired_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.event_id(), EventId{"event-123"});
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
}

TEST(ReservationExpiredEventTest, Constructor_ShouldStorePreviousStatus_WhenConfirmedReservationExpires) {
    const ReservationExpiredEvent event = create_expired_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.previous_status(), ReservationStatus::Confirmed);
}

TEST(ReservationExpiredEventTest, Constructor_ShouldAllowPendingApprovalAsPreviousStatus) {
    const ReservationExpiredEvent event = create_expired_event(ReservationStatus::PendingApproval);

    EXPECT_EQ(event.previous_status(), ReservationStatus::PendingApproval);
}

TEST(ReservationExpiredEventTest, Constructor_ShouldStoreOccurrenceTime_WhenEventIsCreated) {
    const ReservationExpiredEvent event = create_expired_event(ReservationStatus::Confirmed);

    EXPECT_EQ(event.occurred_at(), ReservationExpiredEvent::TimePoint{} + 30min);
}

TEST(ReservationExpiredEventTest, Constructor_ShouldThrow_WhenPreviousStatusIsTerminal) {
    EXPECT_THROW(create_expired_event(ReservationStatus::Cancelled), std::invalid_argument);
}

TEST(ReservationExpiredEventTest, Constructor_ShouldThrow_WhenPreviousStatusIsRejected) {
    EXPECT_THROW(create_expired_event(ReservationStatus::Rejected), std::invalid_argument);
}

TEST(ReservationExpiredEventTest, Equality_ShouldReturnTrue_WhenValuesMatch) {
    const ReservationExpiredEvent first = create_expired_event(ReservationStatus::Confirmed);
    const ReservationExpiredEvent second = create_expired_event(ReservationStatus::Confirmed);

    EXPECT_EQ(first, second);
}

TEST(ReservationExpiredEventTest, Equality_ShouldReturnFalse_WhenPreviousStatusesDiffer) {
    const ReservationExpiredEvent first = create_expired_event(ReservationStatus::Confirmed);
    const ReservationExpiredEvent second = create_expired_event(ReservationStatus::PendingApproval);

    EXPECT_NE(first, second);
}

}  // namespace
}  // namespace haven::domain