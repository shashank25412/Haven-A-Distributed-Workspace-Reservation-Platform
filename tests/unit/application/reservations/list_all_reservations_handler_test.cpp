/**
 * @file list_all_reservations_handler_test.cpp
 * @brief Tests ListAllReservations application orchestration.
 */

#include "haven/application/reservations/list_all_reservations_handler.hpp"

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
        const OrganizationId&, const ReservationId&) const override {
        return std::nullopt;
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

    [[nodiscard]] ReservationListResult find_all(
        const OrganizationId& organization_id) const override {
        auto result = ReservationListResult{};

        for (const auto& reservation : reservations_) {
            if (reservation.organization_id() == organization_id) {
                result.push_back(reservation);
            }
        }

        return result;
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

    [[nodiscard]] persistence::PersistenceToken insert(const OrganizationId&,
                                                       const Reservation&) override {
        return persistence::PersistenceToken{1};
    }
    [[nodiscard]] persistence::PersistenceToken update(
        const OrganizationId&, const Reservation&, const persistence::PersistenceToken&) override {
        return persistence::PersistenceToken{2};
    }

private:
    std::vector<Reservation> reservations_;
};

class AllReservationsLeakingRepository final : public ReservationRepository {
public:
    explicit AllReservationsLeakingRepository(ReservationListResult reservations)
        : reservations_(std::move(reservations)) {}

    [[nodiscard]] ReservationLookupResult find_by_id(
        const OrganizationId&, const ReservationId&) const override {
        return std::nullopt;
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

    [[nodiscard]] ReservationListResult find_all(const OrganizationId&) const override {
        return reservations_;
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

    [[nodiscard]] persistence::PersistenceToken insert(const OrganizationId&,
                                                       const Reservation&) override {
        return persistence::PersistenceToken{1};
    }
    [[nodiscard]] persistence::PersistenceToken update(
        const OrganizationId&, const Reservation&, const persistence::PersistenceToken&) override {
        return persistence::PersistenceToken{2};
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

TEST(ListAllReservationsHandlerTest, Handle_ShouldReturnEveryReservation_RegardlessOfCreator) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_reservation(ReservationId{"reservation-100"},
                                    organization_id,
                                    ResourceId{"resource-boardroom"},
                                    UserId{"user-100"}));
    repository.add(make_reservation(ReservationId{"reservation-101"},
                                    organization_id,
                                    ResourceId{"resource-huddle-room"},
                                    UserId{"user-200"}));
    const auto handler = ListAllReservationsHandler{repository};

    const auto result = handler.handle(ListAllReservationsQuery{organization_id});

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result.at(0).organization_id(), organization_id);
    EXPECT_EQ(result.at(1).organization_id(), organization_id);
}

TEST(ListAllReservationsHandlerTest, Handle_ShouldReturnEmpty_WhenOrganizationHasNoReservations) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    const auto handler = ListAllReservationsHandler{repository};

    const auto result = handler.handle(ListAllReservationsQuery{organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListAllReservationsHandlerTest, Handle_ShouldExcludeReservationsFromAnotherOrganization) {
    const auto caller_organization_id = OrganizationId{"organization-alpha"};
    const auto owner_organization_id = OrganizationId{"organization-beta"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_reservation(ReservationId{"reservation-100"},
                                    owner_organization_id,
                                    ResourceId{"resource-boardroom"},
                                    UserId{"user-100"}));
    const auto handler = ListAllReservationsHandler{repository};

    const auto result = handler.handle(ListAllReservationsQuery{caller_organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListAllReservationsHandlerTest,
     Handle_ShouldRemoveReservationsOutsideOrganization_WhenRepositoryLeaksData) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto other_organization_id = OrganizationId{"organization-beta"};
    auto repository = AllReservationsLeakingRepository{
        ReservationListResult{make_reservation(ReservationId{"reservation-100"},
                                               organization_id,
                                               ResourceId{"resource-boardroom"},
                                               UserId{"user-100"}),
                              make_reservation(ReservationId{"reservation-200"},
                                               other_organization_id,
                                               ResourceId{"resource-boardroom"},
                                               UserId{"user-200"})}};
    const auto handler = ListAllReservationsHandler{repository};

    const auto result = handler.handle(ListAllReservationsQuery{organization_id});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.at(0).organization_id(), organization_id);
}

}  // namespace
}  // namespace haven::application::reservations
