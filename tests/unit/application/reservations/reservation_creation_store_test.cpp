/**
 * @file reservation_creation_store_test.cpp
 * @brief Verifies the application test store for atomic reservation creation.
 */

#include "haven/application/repository_error.hpp"
#include "haven/domain/events/reservation_approval_requested_event.hpp"
#include "haven/domain/events/reservation_confirmed_event.hpp"
#include "haven/domain/events/reservation_created_event.hpp"

#include "application/util/test_reservation_creation_store.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <variant>

namespace haven::tests::util::application {
namespace {

using namespace std::chrono_literals;

constexpr auto kOccurredAt = haven::domain::Reservation::TimePoint{} + 30min;
const auto kOrganizationId = haven::domain::OrganizationId{"organization-1"};

[[nodiscard]] haven::domain::Reservation create_confirmed_reservation() {
    return haven::domain::Reservation::create_confirmed(
        kOrganizationId,
        haven::domain::ReservationId{"reservation-confirmed"},
        haven::domain::ResourceId{"resource-1"},
        haven::domain::UserId{"user-1"},
        haven::domain::TimeInterval{kOccurredAt + 1h, kOccurredAt + 2h},
        haven::domain::Purpose{"Planning"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created"},
        haven::domain::EventId{"event-confirmed"},
        kOccurredAt);
}

[[nodiscard]] haven::domain::Reservation create_pending_reservation() {
    return haven::domain::Reservation::create_pending_approval(
        kOrganizationId,
        haven::domain::ReservationId{"reservation-pending"},
        haven::domain::ResourceId{"resource-2"},
        haven::domain::UserId{"user-1"},
        haven::domain::TimeInterval{kOccurredAt + 3h, kOccurredAt + 4h},
        haven::domain::Purpose{"Leadership review"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created-pending"},
        haven::domain::EventId{"event-approval-requested"},
        kOccurredAt);
}

TEST(ReservationCreationStoreTest, PersistsConfirmedReservationAndOrderedCreationEvents) {
    auto store = TestReservationCreationStore{};
    const auto configured_token = haven::application::persistence::PersistenceToken{42};
    store.set_persistence_token(configured_token);
    auto reservation = create_confirmed_reservation();
    auto events = reservation.release_domain_events();

    const auto returned_token = store.persist(kOrganizationId, reservation, std::move(events));

    EXPECT_EQ(returned_token, configured_token);
    EXPECT_EQ(store.persist_call_count(), 1U);
    ASSERT_EQ(store.persisted_organization_id(), kOrganizationId);
    ASSERT_TRUE(store.persisted_reservation().has_value());
    EXPECT_EQ(store.persisted_reservation()->reservation_id(), reservation.reservation_id());
    EXPECT_EQ(store.persisted_reservation()->status(), haven::domain::ReservationStatus::Confirmed);
    ASSERT_EQ(store.persisted_domain_events().size(), 2U);
    ASSERT_TRUE(std::holds_alternative<haven::domain::ReservationCreatedEvent>(
        store.persisted_domain_events()[0]));
    ASSERT_TRUE(std::holds_alternative<haven::domain::ReservationConfirmedEvent>(
        store.persisted_domain_events()[1]));

    const auto& created =
        std::get<haven::domain::ReservationCreatedEvent>(store.persisted_domain_events()[0]);
    const auto& confirmed =
        std::get<haven::domain::ReservationConfirmedEvent>(store.persisted_domain_events()[1]);
    EXPECT_EQ(created.event_id(), haven::domain::EventId{"event-created"});
    EXPECT_EQ(created.occurred_at(), kOccurredAt);
    EXPECT_EQ(confirmed.event_id(), haven::domain::EventId{"event-confirmed"});
    EXPECT_EQ(confirmed.occurred_at(), kOccurredAt);
}

TEST(ReservationCreationStoreTest, PersistsPendingReservationAndOrderedCreationEvents) {
    auto store = TestReservationCreationStore{};
    auto reservation = create_pending_reservation();
    auto events = reservation.release_domain_events();

    static_cast<void>(store.persist(kOrganizationId, reservation, std::move(events)));

    ASSERT_TRUE(store.persisted_reservation().has_value());
    EXPECT_EQ(store.persisted_reservation()->reservation_id(), reservation.reservation_id());
    EXPECT_EQ(store.persisted_reservation()->status(),
              haven::domain::ReservationStatus::PendingApproval);
    ASSERT_EQ(store.persisted_domain_events().size(), 2U);
    ASSERT_TRUE(std::holds_alternative<haven::domain::ReservationCreatedEvent>(
        store.persisted_domain_events()[0]));
    ASSERT_TRUE(std::holds_alternative<haven::domain::ReservationApprovalRequestedEvent>(
        store.persisted_domain_events()[1]));

    const auto& created =
        std::get<haven::domain::ReservationCreatedEvent>(store.persisted_domain_events()[0]);
    const auto& approval_requested = std::get<haven::domain::ReservationApprovalRequestedEvent>(
        store.persisted_domain_events()[1]);
    EXPECT_EQ(created.event_id(), haven::domain::EventId{"event-created-pending"});
    EXPECT_EQ(created.occurred_at(), kOccurredAt);
    EXPECT_EQ(approval_requested.event_id(), haven::domain::EventId{"event-approval-requested"});
    EXPECT_EQ(approval_requested.occurred_at(), kOccurredAt);
}

TEST(ReservationCreationStoreTest, PropagatesConfiguredFailureWithoutRecordingPersistence) {
    auto store = TestReservationCreationStore{};
    store.force_failure();
    auto reservation = create_confirmed_reservation();
    auto events = reservation.release_domain_events();

    try {
        static_cast<void>(store.persist(kOrganizationId, reservation, std::move(events)));
        FAIL() << "Expected persistence failure";
    } catch (const haven::application::RepositoryError& error) {
        EXPECT_EQ(error.code(), haven::application::RepositoryErrorCode::Persistence);
    }

    EXPECT_EQ(store.persist_call_count(), 1U);
    EXPECT_FALSE(store.persisted_organization_id().has_value());
    EXPECT_FALSE(store.persisted_reservation().has_value());
    EXPECT_TRUE(store.persisted_domain_events().empty());
}

}  // namespace
}  // namespace haven::tests::util::application
