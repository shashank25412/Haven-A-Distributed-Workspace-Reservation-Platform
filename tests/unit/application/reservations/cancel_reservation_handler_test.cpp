/**
 * @file cancel_reservation_handler_test.cpp
 * @brief Tests CancelReservation application orchestration.
 */

#include "haven/application/reservations/cancel_reservation_handler.hpp"

#include "haven/domain/reservation.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>
#include <vector>

namespace haven::application::reservations {
namespace {

class InMemoryReservationRepository final : public ReservationRepository {
public:
    void add(haven::domain::Reservation reservation) {
        reservations_.push_back(std::move(reservation));
    }

    [[nodiscard]] ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id) const override {
        const auto reservation = std::find_if(
            reservations_.cbegin(),
            reservations_.cend(),
            [&organization_id, &reservation_id](const haven::domain::Reservation& candidate) {
                return candidate.organization_id() == organization_id &&
                       candidate.reservation_id() == reservation_id;
            });

        if (reservation == reservations_.cend()) {
            return std::nullopt;
        }

        return LoadedReservation{*reservation, persistence::PersistenceToken{1}};
    }

    [[nodiscard]] ReservationListResult find_by_creator(
        const haven::domain::OrganizationId&, const haven::domain::UserId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
        const haven::domain::OrganizationId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_by_resource_and_interval(
        const haven::domain::OrganizationId&,
        const haven::domain::ResourceId&,
        const haven::domain::TimeInterval&) const override {
        return {};
    }

    [[nodiscard]] bool has_conflict(const haven::domain::OrganizationId&,
                                    const haven::domain::ResourceId&,
                                    const haven::domain::TimeInterval&) const override {
        return false;
    }

    [[nodiscard]] bool has_conflict_excluding(const haven::domain::OrganizationId&,
                                              const haven::domain::ResourceId&,
                                              const haven::domain::TimeInterval&,
                                              const haven::domain::ReservationId&) const override {
        return false;
    }

    [[nodiscard]] persistence::PersistenceToken insert(const haven::domain::OrganizationId&,
                                                       const haven::domain::Reservation&) override {
        return persistence::PersistenceToken{1};
    }
    [[nodiscard]] persistence::PersistenceToken update(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::Reservation& reservation,
        const persistence::PersistenceToken&) override {
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
        return persistence::PersistenceToken{2};
    }

    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& saved_organization_id()
        const noexcept {
        return saved_organization_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::Reservation>& saved_reservation()
        const noexcept {
        return saved_reservation_;
    }

private:
    std::vector<haven::domain::Reservation> reservations_;
    std::optional<haven::domain::OrganizationId> saved_organization_id_;
    std::optional<haven::domain::Reservation> saved_reservation_;
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] haven::domain::Reservation make_confirmed_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::UserId& creator_id) {
    return haven::domain::Reservation::create_confirmed(
        organization_id,
        reservation_id,
        resource_id,
        creator_id,
        haven::domain::TimeInterval{make_time_point(10), make_time_point(11)},
        haven::domain::Purpose{"Planning meeting"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created-100"},
        haven::domain::EventId{"event-confirmed-100"},
        make_time_point(9));
}

[[nodiscard]] CancelReservationCommand make_command(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::UserId& caller_id) {
    return CancelReservationCommand{organization_id,
                                    reservation_id,
                                    caller_id,
                                    haven::domain::EventId{"event-cancelled-100"},
                                    make_time_point(9)};
}

TEST(CancelReservationHandlerTest, Handle_ShouldCancelReservation_WhenCallerOwnsReservation) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto caller_id = haven::domain::UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_confirmed_reservation(reservation_id,
                                              organization_id,
                                              haven::domain::ResourceId{"resource-boardroom"},
                                              caller_id));
    const auto handler = CancelReservationHandler{repository};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    ASSERT_EQ(result.status(), CancelReservationStatus::CANCELLED);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->status(), haven::domain::ReservationStatus::Cancelled);
    ASSERT_TRUE(repository.saved_reservation().has_value());
    EXPECT_EQ(repository.saved_reservation()->status(),
              haven::domain::ReservationStatus::Cancelled);
    EXPECT_EQ(repository.saved_organization_id(), organization_id);
}

TEST(CancelReservationHandlerTest, Handle_ShouldReturnNotFound_WhenReservationDoesNotExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-missing"};
    const auto caller_id = haven::domain::UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto handler = CancelReservationHandler{repository};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), CancelReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(CancelReservationHandlerTest,
     Handle_ShouldReturnNotFound_WhenReservationBelongsToAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto caller_id = haven::domain::UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_confirmed_reservation(reservation_id,
                                              owner_organization_id,
                                              haven::domain::ResourceId{"resource-boardroom"},
                                              caller_id));
    const auto handler = CancelReservationHandler{repository};

    const auto result =
        handler.handle(make_command(caller_organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), CancelReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(CancelReservationHandlerTest,
     Handle_ShouldRejectCancellation_WhenCallerDoesNotOwnReservation) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto creator_id = haven::domain::UserId{"user-100"};
    const auto caller_id = haven::domain::UserId{"user-200"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_confirmed_reservation(reservation_id,
                                              organization_id,
                                              haven::domain::ResourceId{"resource-boardroom"},
                                              creator_id));
    const auto handler = CancelReservationHandler{repository};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), CancelReservationStatus::CALLER_NOT_AUTHORIZED);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(CancelReservationHandlerTest,
     Handle_ShouldRejectCancellation_WhenReservationIsAlreadyCancelled) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto caller_id = haven::domain::UserId{"user-100"};
    auto reservation = make_confirmed_reservation(reservation_id,
                                                  organization_id,
                                                  haven::domain::ResourceId{"resource-boardroom"},
                                                  caller_id);
    reservation.cancel(
        caller_id, make_time_point(9), haven::domain::EventId{"event-cancelled-original"});
    auto repository = InMemoryReservationRepository{};
    repository.add(std::move(reservation));
    const auto handler = CancelReservationHandler{repository};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), CancelReservationStatus::INVALID_STATE);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

}  // namespace
}  // namespace haven::application::reservations
