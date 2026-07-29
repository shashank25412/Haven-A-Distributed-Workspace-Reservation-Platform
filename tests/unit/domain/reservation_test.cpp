/**
 * @file reservation_test.cpp
 * @brief Tests the reservation domain aggregate.
 */

#include "haven/domain/reservation.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <variant>
#include <vector>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

constexpr auto kCreatedAt = Reservation::TimePoint{} + 30min;

Reservation create_confirmed_reservation() {
    const TimeInterval::TimePoint start{};

    return Reservation::create_confirmed(OrganizationId{"organization-123"},
                                         ReservationId{"reservation-123"},
                                         ResourceId{"resource-123"},
                                         UserId{"user-123"},
                                         TimeInterval{start, start + 2h},
                                         Purpose{"Team meeting"},
                                         ReservationKind::Standard,
                                         EventId{"event-created-123"},
                                         EventId{"event-confirmed-123"},
                                         kCreatedAt);
}

Reservation create_pending_reservation() {
    const TimeInterval::TimePoint start{};

    return Reservation::create_pending_approval(OrganizationId{"organization-123"},
                                                ReservationId{"reservation-123"},
                                                ResourceId{"resource-123"},
                                                UserId{"user-123"},
                                                TimeInterval{start, start + 2h},
                                                Purpose{"Leadership meeting"},
                                                ReservationKind::Standard,
                                                EventId{"event-created-123"},
                                                EventId{"event-approval-requested-123"},
                                                kCreatedAt);
}

TEST(ReservationTest, CreateConfirmed_ShouldStoreReservationDetails_WhenCreationSucceeds) {
    const Reservation reservation = create_confirmed_reservation();

    EXPECT_EQ(reservation.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(reservation.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(reservation.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(reservation.created_by(), UserId{"user-123"});
    EXPECT_EQ(reservation.purpose(), Purpose{"Team meeting"});
    EXPECT_EQ(reservation.kind(), ReservationKind::Standard);
    EXPECT_EQ(reservation.status(), ReservationStatus::Confirmed);
    EXPECT_FALSE(reservation.approval_info().has_value());
}

TEST(ReservationTest, CreateConfirmed_ShouldRecordCreatedAndConfirmedEvents_WhenCreationSucceeds) {
    Reservation reservation = create_confirmed_reservation();

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 2U);
    ASSERT_TRUE(std::holds_alternative<ReservationCreatedEvent>(events[0]));
    ASSERT_TRUE(std::holds_alternative<ReservationConfirmedEvent>(events[1]));

    const ReservationCreatedEvent& created_event = std::get<ReservationCreatedEvent>(events[0]);
    const ReservationConfirmedEvent& confirmed_event =
        std::get<ReservationConfirmedEvent>(events[1]);

    EXPECT_EQ(created_event.event_id(), EventId{"event-created-123"});
    EXPECT_EQ(created_event.initial_status(), ReservationStatus::Confirmed);
    EXPECT_EQ(confirmed_event.event_id(), EventId{"event-confirmed-123"});
    EXPECT_EQ(confirmed_event.occurred_at(), kCreatedAt);
    EXPECT_FALSE(confirmed_event.confirmed_by().has_value());
}

TEST(ReservationTest, CreatePendingApproval_ShouldSetPendingStatus_WhenResourceRequiresApproval) {
    const Reservation reservation = create_pending_reservation();

    EXPECT_EQ(reservation.status(), ReservationStatus::PendingApproval);
    EXPECT_FALSE(reservation.approval_info().has_value());
}

TEST(ReservationTest,
     CreatePendingApproval_ShouldRecordCreatedAndApprovalRequestedEvents_WhenCreationSucceeds) {
    Reservation reservation = create_pending_reservation();

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 2U);
    ASSERT_TRUE(std::holds_alternative<ReservationCreatedEvent>(events[0]));
    ASSERT_TRUE(std::holds_alternative<ReservationApprovalRequestedEvent>(events[1]));

    const ReservationCreatedEvent& created_event = std::get<ReservationCreatedEvent>(events[0]);
    const ReservationApprovalRequestedEvent& approval_event =
        std::get<ReservationApprovalRequestedEvent>(events[1]);

    EXPECT_EQ(created_event.event_id(), EventId{"event-created-123"});
    EXPECT_EQ(created_event.initial_status(), ReservationStatus::PendingApproval);
    EXPECT_EQ(approval_event.event_id(), EventId{"event-approval-requested-123"});
    EXPECT_EQ(approval_event.reservation_id(), ReservationId{"reservation-123"});
}

TEST(ReservationTest, ReleaseDomainEvents_ShouldRemoveRecordedEvents_WhenEventsAreReleased) {
    Reservation reservation = create_pending_reservation();

    const std::vector<ReservationDomainEvent> first_release = reservation.release_domain_events();
    const std::vector<ReservationDomainEvent> second_release = reservation.release_domain_events();

    EXPECT_EQ(first_release.size(), 2U);
    EXPECT_TRUE(second_release.empty());
}

TEST(ReservationTest, Approve_ShouldConfirmReservation_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();
    static_cast<void>(reservation.release_domain_events());

    const Reservation::TimePoint approved_at = Reservation::TimePoint{} + 1h;

    reservation.approve(UserId{"approver-123"}, approved_at, EventId{"event-confirmed-123"});

    ASSERT_TRUE(reservation.approval_info().has_value());
    EXPECT_EQ(reservation.status(), ReservationStatus::Confirmed);
    EXPECT_EQ(reservation.approval_info()->approved_by(), UserId{"approver-123"});
    EXPECT_EQ(reservation.approval_info()->approved_at(), approved_at);
}

TEST(ReservationTest, Approve_ShouldRecordConfirmedEvent_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();
    static_cast<void>(reservation.release_domain_events());

    const Reservation::TimePoint approved_at = Reservation::TimePoint{} + 1h;

    reservation.approve(UserId{"approver-123"}, approved_at, EventId{"event-confirmed-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationConfirmedEvent>(events.front()));

    const ReservationConfirmedEvent& event = std::get<ReservationConfirmedEvent>(events.front());

    EXPECT_EQ(event.event_id(), EventId{"event-confirmed-123"});
    EXPECT_EQ(event.occurred_at(), approved_at);
    ASSERT_TRUE(event.confirmed_by().has_value());
    EXPECT_EQ(event.confirmed_by().value(), UserId{"approver-123"});
}

TEST(ReservationTest, Approve_ShouldThrow_WhenReservationIsAlreadyConfirmed) {
    Reservation reservation = create_confirmed_reservation();

    EXPECT_THROW(
        reservation.approve(
            UserId{"approver-123"}, Reservation::TimePoint{}, EventId{"event-confirmed-456"}),
        std::logic_error);
}

TEST(ReservationTest, Reject_ShouldRejectReservation_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();

    reservation.reject(
        UserId{"approver-123"}, Reservation::TimePoint{} + 1h, EventId{"event-rejected-123"});

    EXPECT_EQ(reservation.status(), ReservationStatus::Rejected);
}

TEST(ReservationTest, Reject_ShouldRecordRejectedEvent_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();
    static_cast<void>(reservation.release_domain_events());

    const Reservation::TimePoint rejected_at = Reservation::TimePoint{} + 1h;

    reservation.reject(UserId{"approver-123"}, rejected_at, EventId{"event-rejected-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationRejectedEvent>(events.front()));

    const ReservationRejectedEvent& event = std::get<ReservationRejectedEvent>(events.front());

    EXPECT_EQ(event.event_id(), EventId{"event-rejected-123"});
    EXPECT_EQ(event.occurred_at(), rejected_at);
    EXPECT_EQ(event.rejected_by(), UserId{"approver-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
}

TEST(ReservationTest, Reject_ShouldThrow_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();

    EXPECT_THROW(
        reservation.reject(
            UserId{"approver-123"}, Reservation::TimePoint{} + 1h, EventId{"event-rejected-123"}),
        std::logic_error);
}

TEST(ReservationTest, Cancel_ShouldCancelReservation_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();

    reservation.cancel(
        UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-cancelled-123"});

    EXPECT_EQ(reservation.status(), ReservationStatus::Cancelled);
}

TEST(ReservationTest, Cancel_ShouldCancelReservation_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();

    reservation.cancel(
        UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-cancelled-123"});

    EXPECT_EQ(reservation.status(), ReservationStatus::Cancelled);
}

TEST(ReservationTest, Cancel_ShouldRecordCancelledEvent_WhenConfirmedReservationIsCancelled) {
    Reservation reservation = create_confirmed_reservation();
    static_cast<void>(reservation.release_domain_events());

    const Reservation::TimePoint cancelled_at = Reservation::TimePoint{} + 1h;

    reservation.cancel(UserId{"user-123"}, cancelled_at, EventId{"event-cancelled-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationCancelledEvent>(events.front()));

    const ReservationCancelledEvent& event = std::get<ReservationCancelledEvent>(events.front());

    EXPECT_EQ(event.event_id(), EventId{"event-cancelled-123"});
    EXPECT_EQ(event.occurred_at(), cancelled_at);
    EXPECT_EQ(event.cancelled_by(), UserId{"user-123"});
    EXPECT_EQ(event.previous_status(), ReservationStatus::Confirmed);
}

TEST(ReservationTest, Cancel_ShouldRecordPendingStatus_WhenPendingReservationIsCancelled) {
    Reservation reservation = create_pending_reservation();
    static_cast<void>(reservation.release_domain_events());

    reservation.cancel(
        UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-cancelled-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationCancelledEvent>(events.front()));

    const ReservationCancelledEvent& event = std::get<ReservationCancelledEvent>(events.front());

    EXPECT_EQ(event.previous_status(), ReservationStatus::PendingApproval);
}

TEST(ReservationTest, Cancel_ShouldThrow_WhenReservationIsTerminal) {
    Reservation reservation = create_pending_reservation();

    reservation.reject(
        UserId{"approver-123"}, Reservation::TimePoint{} + 1h, EventId{"event-rejected-123"});

    EXPECT_THROW(
        reservation.cancel(
            UserId{"user-123"}, Reservation::TimePoint{} + 2h, EventId{"event-cancelled-123"}),
        std::logic_error);
}

TEST(ReservationTest, Extend_ShouldMoveEndForward_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();
    const TimeInterval::TimePoint new_end = reservation.interval().end() + 1h;

    reservation.extend(
        new_end, UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-extended-123"});

    EXPECT_EQ(reservation.interval().end(), new_end);
}

TEST(ReservationTest, Extend_ShouldRecordExtendedEvent_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();
    static_cast<void>(reservation.release_domain_events());

    const TimeInterval previous_interval = reservation.interval();
    const TimeInterval::TimePoint new_end = previous_interval.end() + 1h;
    const Reservation::TimePoint extended_at = Reservation::TimePoint{} + 1h;

    reservation.extend(new_end, UserId{"user-123"}, extended_at, EventId{"event-extended-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationExtendedEvent>(events.front()));

    const ReservationExtendedEvent& event = std::get<ReservationExtendedEvent>(events.front());

    EXPECT_EQ(event.event_id(), EventId{"event-extended-123"});
    EXPECT_EQ(event.occurred_at(), extended_at);
    EXPECT_EQ(event.extended_by(), UserId{"user-123"});
    EXPECT_EQ(event.previous_interval(), previous_interval);
    EXPECT_EQ(event.extended_interval(), reservation.interval());
    EXPECT_EQ(event.extended_interval().end(), new_end);
}

TEST(ReservationTest, Extend_ShouldThrow_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();

    EXPECT_THROW(reservation.extend(reservation.interval().end() + 1h,
                                    UserId{"user-123"},
                                    Reservation::TimePoint{} + 1h,
                                    EventId{"event-extended-123"}),
                 std::logic_error);
}

TEST(ReservationTest, Extend_ShouldThrow_WhenNewEndEqualsCurrentEnd) {
    Reservation reservation = create_confirmed_reservation();

    EXPECT_THROW(reservation.extend(reservation.interval().end(),
                                    UserId{"user-123"},
                                    Reservation::TimePoint{} + 1h,
                                    EventId{"event-extended-123"}),
                 std::invalid_argument);
}

TEST(ReservationTest, Extend_ShouldThrow_WhenNewEndIsBeforeCurrentEnd) {
    Reservation reservation = create_confirmed_reservation();

    EXPECT_THROW(reservation.extend(reservation.interval().end() - 30min,
                                    UserId{"user-123"},
                                    Reservation::TimePoint{} + 1h,
                                    EventId{"event-extended-123"}),
                 std::invalid_argument);
}

TEST(ReservationTest, Extend_ShouldNotRecordEvent_WhenExtensionIsRejected) {
    Reservation reservation = create_confirmed_reservation();
    static_cast<void>(reservation.release_domain_events());

    EXPECT_THROW(reservation.extend(reservation.interval().end(),
                                    UserId{"user-123"},
                                    Reservation::TimePoint{} + 1h,
                                    EventId{"event-extended-123"}),
                 std::invalid_argument);

    EXPECT_TRUE(reservation.release_domain_events().empty());
}

TEST(ReservationTest, Expire_ShouldExpireReservation_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();

    reservation.expire(Reservation::TimePoint{} + 3h, EventId{"event-expired-123"});

    EXPECT_EQ(reservation.status(), ReservationStatus::Expired);
}

TEST(ReservationTest, Expire_ShouldExpireReservation_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();

    reservation.expire(Reservation::TimePoint{} + 3h, EventId{"event-expired-123"});

    EXPECT_EQ(reservation.status(), ReservationStatus::Expired);
}

TEST(ReservationTest, Expire_ShouldRecordExpiredEvent_WhenConfirmedReservationExpires) {
    Reservation reservation = create_confirmed_reservation();
    static_cast<void>(reservation.release_domain_events());

    const Reservation::TimePoint expired_at = Reservation::TimePoint{} + 3h;

    reservation.expire(expired_at, EventId{"event-expired-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationExpiredEvent>(events.front()));

    const ReservationExpiredEvent& event = std::get<ReservationExpiredEvent>(events.front());

    EXPECT_EQ(event.event_id(), EventId{"event-expired-123"});
    EXPECT_EQ(event.occurred_at(), expired_at);
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(event.previous_status(), ReservationStatus::Confirmed);
}

TEST(ReservationTest, Expire_ShouldRecordPendingStatus_WhenPendingReservationExpires) {
    Reservation reservation = create_pending_reservation();
    static_cast<void>(reservation.release_domain_events());

    reservation.expire(Reservation::TimePoint{} + 3h, EventId{"event-expired-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationExpiredEvent>(events.front()));

    const ReservationExpiredEvent& event = std::get<ReservationExpiredEvent>(events.front());

    EXPECT_EQ(event.previous_status(), ReservationStatus::PendingApproval);
}

TEST(ReservationTest, Expire_ShouldThrow_WhenReservationIsTerminal) {
    Reservation reservation = create_confirmed_reservation();

    reservation.cancel(
        UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-cancelled-123"});

    EXPECT_THROW(reservation.expire(Reservation::TimePoint{} + 3h, EventId{"event-expired-123"}),
                 std::logic_error);
}

TEST(ReservationTest, Expire_ShouldNotRecordEvent_WhenExpirationIsRejected) {
    Reservation reservation = create_confirmed_reservation();
    static_cast<void>(reservation.release_domain_events());

    reservation.cancel(
        UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-cancelled-123"});
    static_cast<void>(reservation.release_domain_events());

    EXPECT_THROW(reservation.expire(Reservation::TimePoint{} + 3h, EventId{"event-expired-123"}),
                 std::logic_error);

    EXPECT_TRUE(reservation.release_domain_events().empty());
}

TEST(ReservationTest, Complete_ShouldCompleteReservation_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();

    reservation.complete(Reservation::TimePoint{} + 3h, EventId{"event-completed-123"});

    EXPECT_EQ(reservation.status(), ReservationStatus::Completed);
}

TEST(ReservationTest, Complete_ShouldRecordCompletedEvent_WhenReservationIsConfirmed) {
    Reservation reservation = create_confirmed_reservation();
    static_cast<void>(reservation.release_domain_events());

    const Reservation::TimePoint completed_at = Reservation::TimePoint{} + 3h;

    reservation.complete(completed_at, EventId{"event-completed-123"});

    const std::vector<ReservationDomainEvent> events = reservation.release_domain_events();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ReservationCompletedEvent>(events.front()));

    const ReservationCompletedEvent& event = std::get<ReservationCompletedEvent>(events.front());

    EXPECT_EQ(event.event_id(), EventId{"event-completed-123"});
    EXPECT_EQ(event.occurred_at(), completed_at);
    EXPECT_EQ(event.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(event.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(event.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(event.interval(), reservation.interval());
}

TEST(ReservationTest, Complete_ShouldThrow_WhenReservationIsPending) {
    Reservation reservation = create_pending_reservation();

    EXPECT_THROW(
        reservation.complete(Reservation::TimePoint{} + 3h, EventId{"event-completed-123"}),
        std::logic_error);
}

TEST(ReservationTest, Complete_ShouldThrow_WhenReservationIsCancelled) {
    Reservation reservation = create_confirmed_reservation();

    reservation.cancel(
        UserId{"user-123"}, Reservation::TimePoint{} + 1h, EventId{"event-cancelled-123"});

    EXPECT_THROW(
        reservation.complete(Reservation::TimePoint{} + 3h, EventId{"event-completed-123"}),
        std::logic_error);
}

TEST(ReservationTest, Complete_ShouldNotRecordEvent_WhenCompletionIsRejected) {
    Reservation reservation = create_pending_reservation();
    static_cast<void>(reservation.release_domain_events());

    EXPECT_THROW(
        reservation.complete(Reservation::TimePoint{} + 3h, EventId{"event-completed-123"}),
        std::logic_error);

    EXPECT_TRUE(reservation.release_domain_events().empty());
}

TEST(ReservationTest, CreateConfirmed_ShouldUseVersionOne_WhenReservationIsNew) {
    const Reservation reservation = create_confirmed_reservation();

    EXPECT_EQ(reservation.version(), Version{1});
}

TEST(ReservationTest, CreatePendingApproval_ShouldUseVersionOne_WhenReservationIsNew) {
    const Reservation reservation = create_pending_reservation();

    EXPECT_EQ(reservation.version(), Version{1});
}

TEST(ReservationTest, Rehydrate_ShouldRestorePersistedState_WhenReservationExists) {
    const TimeInterval::TimePoint start{};
    const ApprovalInfo approval_info{UserId{"approver-123"}, Reservation::TimePoint{} + 1h};

    const Reservation reservation = Reservation::rehydrate(OrganizationId{"organization-123"},
                                                           ReservationId{"reservation-123"},
                                                           ResourceId{"resource-123"},
                                                           UserId{"user-123"},
                                                           TimeInterval{start, start + 2h},
                                                           Purpose{"Team meeting"},
                                                           ReservationKind::Standard,
                                                           ReservationStatus::Confirmed,
                                                           approval_info,
                                                           Version{42});

    EXPECT_EQ(reservation.organization_id(), OrganizationId{"organization-123"});
    EXPECT_EQ(reservation.reservation_id(), ReservationId{"reservation-123"});
    EXPECT_EQ(reservation.resource_id(), ResourceId{"resource-123"});
    EXPECT_EQ(reservation.created_by(), UserId{"user-123"});
    EXPECT_EQ(reservation.interval(), TimeInterval(start, start + 2h));
    EXPECT_EQ(reservation.purpose(), Purpose{"Team meeting"});
    EXPECT_EQ(reservation.kind(), ReservationKind::Standard);
    EXPECT_EQ(reservation.status(), ReservationStatus::Confirmed);
    EXPECT_EQ(reservation.approval_info(), approval_info);
    EXPECT_EQ(reservation.version(), Version{42});
}

TEST(ReservationTest, Rehydrate_ShouldNotRecordDomainEvents_WhenRestoringPersistedState) {
    const TimeInterval::TimePoint start{};

    Reservation reservation = Reservation::rehydrate(OrganizationId{"organization-123"},
                                                     ReservationId{"reservation-123"},
                                                     ResourceId{"resource-123"},
                                                     UserId{"user-123"},
                                                     TimeInterval{start, start + 2h},
                                                     Purpose{""},
                                                     ReservationKind::Standard,
                                                     ReservationStatus::Cancelled,
                                                     std::nullopt,
                                                     Version{42});

    EXPECT_TRUE(reservation.release_domain_events().empty());
}

TEST(ReservationTest, Rehydrate_ShouldRejectZeroPersistenceVersion) {
    const TimeInterval::TimePoint start{};

    EXPECT_THROW(static_cast<void>(Reservation::rehydrate(OrganizationId{"organization-123"},
                                                          ReservationId{"reservation-123"},
                                                          ResourceId{"resource-123"},
                                                          UserId{"user-123"},
                                                          TimeInterval{start, start + 2h},
                                                          Purpose{""},
                                                          ReservationKind::Standard,
                                                          ReservationStatus::Confirmed,
                                                          std::nullopt,
                                                          Version{0})),
                 std::invalid_argument);
}

TEST(ReservationTest, SuccessfulMutations_ShouldProgressVersionMonotonically) {
    Reservation reservation = create_pending_reservation();

    reservation.approve(UserId{"approver-123"}, Reservation::TimePoint{} + 1h, EventId{"approved"});
    EXPECT_EQ(reservation.version(), Version{2});

    reservation.extend(Reservation::TimePoint{} + 3h,
                       UserId{"user-123"},
                       Reservation::TimePoint{} + 1h,
                       EventId{"extended"});
    EXPECT_EQ(reservation.version(), Version{3});

    reservation.complete(Reservation::TimePoint{} + 4h, EventId{"completed"});
    EXPECT_EQ(reservation.version(), Version{4});
}

TEST(ReservationTest, RejectedMutation_ShouldLeaveVersionUnchanged) {
    Reservation reservation = create_pending_reservation();

    EXPECT_THROW(reservation.complete(Reservation::TimePoint{} + 3h, EventId{"completed"}),
                 std::logic_error);

    EXPECT_EQ(reservation.version(), Version{1});
}

}  // namespace
}  // namespace haven::domain
