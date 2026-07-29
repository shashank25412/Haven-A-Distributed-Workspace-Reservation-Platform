/**
 * @file list_caller_reservations_handler_test.cpp
 * @brief Tests ListCallerReservations application orchestration.
 */

#include "haven/application/reservations/list_caller_reservations_handler.hpp"

#include "haven/domain/reservation.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
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
using haven::domain::ResourceId;
using haven::domain::TimeInterval;
using haven::domain::UserId;

class InMemoryReservationRepository final : public ReservationRepository {
public:
    void add(Reservation reservation) {
        reservations_.push_back(std::move(reservation));
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

    [[nodiscard]] ReservationListResult find_by_creator(const OrganizationId& organization_id,
                                                        const UserId& caller_id) const override {
        auto result = ReservationListResult{};

        for (const auto& reservation : reservations_) {
            if (reservation.organization_id() == organization_id &&
                reservation.created_by() == caller_id) {
                result.push_back(reservation);
            }
        }

        return result;
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
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

    [[nodiscard]] bool has_conflict_excluding(const OrganizationId&,
                                              const ResourceId&,
                                              const TimeInterval&,
                                              const ReservationId&) const override {
        return false;
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const OrganizationId&, const Reservation&) override {
        return haven::application::persistence::PersistenceToken{1};
    }
    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const OrganizationId&,
        const Reservation&,
        const haven::application::persistence::PersistenceToken&) override {
        return haven::application::persistence::PersistenceToken{2};
    }

private:
    std::vector<Reservation> reservations_;
};

class CallerLeakingReservationRepository final : public ReservationRepository {
public:
    explicit CallerLeakingReservationRepository(ReservationListResult reservations)
        : reservations_(std::move(reservations)) {}

    [[nodiscard]] ReservationLookupResult find_by_id(const OrganizationId&,
                                                     const ReservationId&) const override {
        return std::nullopt;
    }

    [[nodiscard]] ReservationListResult find_by_creator(const OrganizationId&,
                                                        const UserId&) const override {
        return reservations_;
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
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

    [[nodiscard]] bool has_conflict_excluding(const OrganizationId&,
                                              const ResourceId&,
                                              const TimeInterval&,
                                              const ReservationId&) const override {
        return false;
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const OrganizationId&, const Reservation&) override {
        return haven::application::persistence::PersistenceToken{1};
    }
    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const OrganizationId&,
        const Reservation&,
        const haven::application::persistence::PersistenceToken&) override {
        return haven::application::persistence::PersistenceToken{2};
    }

private:
    ReservationListResult reservations_;
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] Reservation make_reservation(const ReservationId& reservation_id,
                                           const OrganizationId& organization_id,
                                           const ResourceId& resource_id,
                                           const UserId& creator_id) {
    return Reservation::create_confirmed(organization_id,
                                         reservation_id,
                                         resource_id,
                                         creator_id,
                                         TimeInterval{make_time_point(10), make_time_point(11)},
                                         Purpose{"Planning meeting"},
                                         ReservationKind::Standard,
                                         EventId{"event-created-" + reservation_id.value()},
                                         EventId{"event-confirmed-" + reservation_id.value()},
                                         make_time_point(9));
}

TEST(ListCallerReservationsHandlerTest, Handle_ShouldReturnCallerReservations_WhenMatchesExist) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_reservation(ReservationId{"reservation-100"},
                                    organization_id,
                                    ResourceId{"resource-boardroom"},
                                    caller_id));
    repository.add(make_reservation(ReservationId{"reservation-101"},
                                    organization_id,
                                    ResourceId{"resource-huddle-room"},
                                    caller_id));
    const auto handler = ListCallerReservationsHandler{repository};

    const auto result = handler.handle(ListCallerReservationsQuery{organization_id, caller_id});

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result.at(0).organization_id(), organization_id);
    EXPECT_EQ(result.at(0).created_by(), caller_id);
    EXPECT_EQ(result.at(1).organization_id(), organization_id);
    EXPECT_EQ(result.at(1).created_by(), caller_id);
}

TEST(ListCallerReservationsHandlerTest, Handle_ShouldReturnEmpty_WhenCallerHasNoReservations) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    const auto handler = ListCallerReservationsHandler{repository};

    const auto result = handler.handle(ListCallerReservationsQuery{organization_id, caller_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListCallerReservationsHandlerTest, Handle_ShouldExcludeReservationsFromAnotherOrganization) {
    const auto caller_organization_id = OrganizationId{"organization-alpha"};
    const auto owner_organization_id = OrganizationId{"organization-beta"};
    const auto caller_id = UserId{"user-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_reservation(ReservationId{"reservation-100"},
                                    owner_organization_id,
                                    ResourceId{"resource-boardroom"},
                                    caller_id));
    const auto handler = ListCallerReservationsHandler{repository};

    const auto result =
        handler.handle(ListCallerReservationsQuery{caller_organization_id, caller_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListCallerReservationsHandlerTest, Handle_ShouldExcludeReservationsCreatedByAnotherUser) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto caller_id = UserId{"user-100"};
    const auto other_user_id = UserId{"user-200"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_reservation(ReservationId{"reservation-100"},
                                    organization_id,
                                    ResourceId{"resource-boardroom"},
                                    other_user_id));
    const auto handler = ListCallerReservationsHandler{repository};

    const auto result = handler.handle(ListCallerReservationsQuery{organization_id, caller_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListCallerReservationsHandlerTest,
     Handle_ShouldRemoveReservationsOutsideCallerScope_WhenRepositoryLeaksData) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto caller_id = UserId{"user-100"};
    const auto other_user_id = UserId{"user-200"};
    auto repository = CallerLeakingReservationRepository{
        ReservationListResult{make_reservation(ReservationId{"reservation-100"},
                                               organization_id,
                                               ResourceId{"resource-boardroom"},
                                               caller_id),
                              make_reservation(ReservationId{"reservation-101"},
                                               organization_id,
                                               ResourceId{"resource-huddle-room"},
                                               other_user_id),
                              make_reservation(ReservationId{"reservation-102"},
                                               OrganizationId{"organization-beta"},
                                               ResourceId{"resource-training-room"},
                                               caller_id)}};
    const auto handler = ListCallerReservationsHandler{repository};

    const auto result = handler.handle(ListCallerReservationsQuery{organization_id, caller_id});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().organization_id(), organization_id);
    EXPECT_EQ(result.front().created_by(), caller_id);
}

}  // namespace
}  // namespace haven::application::reservations
