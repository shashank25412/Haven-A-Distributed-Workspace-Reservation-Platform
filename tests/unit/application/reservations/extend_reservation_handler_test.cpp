/**
 * @file extend_reservation_handler_test.cpp
 * @brief Tests ExtendReservation application orchestration.
 */

#include "haven/application/reservations/extend_reservation_handler.hpp"

#include "haven/domain/policies/reservation_policy.hpp"
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

using haven::domain::EventId;
using haven::domain::OrganizationId;
using haven::domain::Purpose;
using haven::domain::Reservation;
using haven::domain::ReservationId;
using haven::domain::ReservationKind;
using haven::domain::ReservationPolicy;
using haven::domain::ReservationStatus;
using haven::domain::ResourceId;
using haven::domain::TimeInterval;
using haven::domain::UserId;

class InMemoryReservationRepository final : public ReservationRepository {
public:
    void add(Reservation reservation) {
        reservations_.push_back(std::move(reservation));
    }

    void set_conflict(const bool conflict) noexcept {
        conflict_ = conflict;
    }

    [[nodiscard]] ReservationLookupResult find_by_id(
        const OrganizationId& organization_id, const ReservationId& reservation_id) const override {
        const auto reservation =
            std::find_if(reservations_.cbegin(),
                         reservations_.cend(),
                         [&organization_id, &reservation_id](const Reservation& candidate) {
                             return candidate.organization_id() == organization_id &&
                                    candidate.reservation_id() == reservation_id;
                         });

        if (reservation == reservations_.cend()) {
            return std::nullopt;
        }

        return LoadedReservation{*reservation, persistence::PersistenceToken{1}};
    }

    [[nodiscard]] ReservationListResult find_by_creator(const OrganizationId&,
                                                        const UserId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
        const OrganizationId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_decided_approvals(
        const OrganizationId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_by_resource_and_interval(
        const OrganizationId&, const ResourceId&, const TimeInterval&) const override {
        return {};
    }

    [[nodiscard]] bool has_conflict(const OrganizationId&,
                                    const ResourceId&,
                                    const TimeInterval&) const override {
        return false;
    }

    [[nodiscard]] bool has_conflict_excluding(
        const OrganizationId& organization_id,
        const ResourceId& resource_id,
        const TimeInterval& interval,
        const ReservationId& excluded_reservation_id) const override {
        checked_organization_id_ = organization_id;
        checked_resource_id_ = resource_id;
        checked_interval_ = interval;
        excluded_reservation_id_ = excluded_reservation_id;
        return conflict_;
    }

    [[nodiscard]] persistence::PersistenceToken insert(const OrganizationId&,
                                                       const Reservation&) override {
        return persistence::PersistenceToken{1};
    }
    [[nodiscard]] persistence::PersistenceToken update(
        const OrganizationId& organization_id,
        const Reservation& reservation,
        const persistence::PersistenceToken&) override {
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
        return persistence::PersistenceToken{2};
    }

    [[nodiscard]] const std::optional<Reservation>& saved_reservation() const noexcept {
        return saved_reservation_;
    }

    [[nodiscard]] const std::optional<ReservationId>& excluded_reservation_id() const noexcept {
        return excluded_reservation_id_;
    }

private:
    std::vector<Reservation> reservations_;
    bool conflict_{false};
    mutable std::optional<OrganizationId> checked_organization_id_;
    mutable std::optional<ResourceId> checked_resource_id_;
    mutable std::optional<TimeInterval> checked_interval_;
    mutable std::optional<ReservationId> excluded_reservation_id_;
    std::optional<OrganizationId> saved_organization_id_;
    std::optional<Reservation> saved_reservation_;
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] TimeInterval make_original_interval() {
    return TimeInterval{make_time_point(10), make_time_point(11)};
}

[[nodiscard]] TimeInterval make_extended_interval() {
    return TimeInterval{make_time_point(10), make_time_point(12)};
}

[[nodiscard]] Reservation make_confirmed_reservation(const ReservationId& reservation_id,
                                                     const OrganizationId& organization_id,
                                                     const ResourceId& resource_id,
                                                     const UserId& creator_id) {
    return Reservation::create_confirmed(organization_id,
                                         reservation_id,
                                         resource_id,
                                         creator_id,
                                         make_original_interval(),
                                         Purpose{"Planning meeting"},
                                         ReservationKind::Standard,
                                         EventId{"event-created-100"},
                                         EventId{"event-confirmed-100"},
                                         make_time_point(9));
}

[[nodiscard]] ExtendReservationCommand make_command(
    const OrganizationId& organization_id,
    const ReservationId& reservation_id,
    const UserId& caller_id,
    TimeInterval interval = make_extended_interval()) {
    return ExtendReservationCommand{organization_id,
                                    reservation_id,
                                    caller_id,
                                    std::move(interval),
                                    EventId{"event-extended-100"},
                                    make_time_point(9)};
}

TEST(ExtendReservationHandlerTest, Handle_ShouldExtendReservation_WhenRequestIsValid) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(
        make_confirmed_reservation(reservation_id, organization_id, resource_id, caller_id));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    ASSERT_EQ(result.status(), ExtendReservationStatus::EXTENDED);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->interval(), make_extended_interval());
    ASSERT_TRUE(repository.saved_reservation().has_value());
    EXPECT_EQ(repository.saved_reservation()->interval(), make_extended_interval());
    EXPECT_EQ(repository.excluded_reservation_id(), reservation_id);
}

TEST(ExtendReservationHandlerTest, Handle_ShouldReturnNotFound_WhenReservationDoesNotExist) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-missing"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), ExtendReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ExtendReservationHandlerTest,
     Handle_ShouldReturnNotFound_WhenReservationBelongsToAnotherOrganization) {
    const auto caller_organization_id = OrganizationId{"organization-alpha"};
    const auto owner_organization_id = OrganizationId{"organization-beta"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(make_confirmed_reservation(
        reservation_id, owner_organization_id, ResourceId{"resource-boardroom"}, caller_id));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result =
        handler.handle(make_command(caller_organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), ExtendReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ExtendReservationHandlerTest, Handle_ShouldRejectExtension_WhenCallerDoesNotOwnReservation) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto creator_id = UserId{"user-100"};
    const auto caller_id = UserId{"user-200"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(make_confirmed_reservation(
        reservation_id, organization_id, ResourceId{"resource-boardroom"}, creator_id));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), ExtendReservationStatus::CALLER_NOT_AUTHORIZED);
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ExtendReservationHandlerTest, Handle_ShouldRejectExtension_WhenAnotherReservationConflicts) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(make_confirmed_reservation(
        reservation_id, organization_id, ResourceId{"resource-boardroom"}, caller_id));
    repository.set_conflict(true);
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), ExtendReservationStatus::SCHEDULE_CONFLICT);
    EXPECT_FALSE(repository.saved_reservation().has_value());
    EXPECT_EQ(repository.excluded_reservation_id(), reservation_id);
}

TEST(ExtendReservationHandlerTest, Handle_ShouldRejectExtension_WhenIntervalViolatesPolicy) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(make_confirmed_reservation(
        reservation_id, organization_id, ResourceId{"resource-boardroom"}, caller_id));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result =
        handler.handle(make_command(organization_id,
                                    reservation_id,
                                    caller_id,
                                    TimeInterval{make_time_point(10), make_time_point(23)}));

    EXPECT_EQ(result.status(), ExtendReservationStatus::POLICY_REJECTED);
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ExtendReservationHandlerTest, Handle_ShouldRejectExtension_WhenEndDoesNotMoveForward) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(make_confirmed_reservation(
        reservation_id, organization_id, ResourceId{"resource-boardroom"}, caller_id));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result = handler.handle(
        make_command(organization_id, reservation_id, caller_id, make_original_interval()));

    EXPECT_EQ(result.status(), ExtendReservationStatus::POLICY_REJECTED);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ExtendReservationHandlerTest, Handle_ShouldRejectExtension_WhenStartTimeChanges) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(make_confirmed_reservation(
        reservation_id, organization_id, ResourceId{"resource-boardroom"}, caller_id));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result =
        handler.handle(make_command(organization_id,
                                    reservation_id,
                                    caller_id,
                                    TimeInterval{make_time_point(9), make_time_point(12)}));

    EXPECT_EQ(result.status(), ExtendReservationStatus::POLICY_REJECTED);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ExtendReservationHandlerTest, Handle_ShouldRejectExtension_WhenReservationIsCancelled) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto reservation_id = ReservationId{"reservation-100"};
    const auto caller_id = UserId{"user-100"};
    auto reservation = make_confirmed_reservation(
        reservation_id, organization_id, ResourceId{"resource-boardroom"}, caller_id);
    reservation.cancel(caller_id, make_time_point(9), EventId{"event-cancelled-100"});
    auto repository = InMemoryReservationRepository{};
    const auto reservation_policy = ReservationPolicy{};
    repository.add(std::move(reservation));
    const auto handler = ExtendReservationHandler{repository, reservation_policy};

    const auto result = handler.handle(make_command(organization_id, reservation_id, caller_id));

    EXPECT_EQ(result.status(), ExtendReservationStatus::INVALID_STATE);
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

}  // namespace
}  // namespace haven::application::reservations
