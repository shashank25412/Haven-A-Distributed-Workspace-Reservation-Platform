/**
 * @file list_pending_approvals_handler_test.cpp
 * @brief Tests ListPendingApprovals application orchestration.
 */

#include "haven/application/reservations/list_pending_approvals_handler.hpp"

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
        const haven::domain::OrganizationId& organization_id) const override {
        auto result = ReservationListResult{};

        for (const auto& reservation : reservations_) {
            if (reservation.organization_id() == organization_id &&
                reservation.status() == haven::domain::ReservationStatus::PendingApproval) {
                result.push_back(reservation);
            }
        }

        return result;
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

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const haven::domain::OrganizationId&, const haven::domain::Reservation&) override {
        return haven::application::persistence::PersistenceToken{1};
    }
    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const haven::domain::OrganizationId&,
        const haven::domain::Reservation&,
        const haven::application::persistence::PersistenceToken&) override {
        return haven::application::persistence::PersistenceToken{2};
    }

private:
    std::vector<haven::domain::Reservation> reservations_;
};

class ApprovalQueueLeakingRepository final : public ReservationRepository {
public:
    explicit ApprovalQueueLeakingRepository(ReservationListResult reservations)
        : reservations_(std::move(reservations)) {}

    [[nodiscard]] ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId&, const haven::domain::ReservationId&) const override {
        return std::nullopt;
    }

    [[nodiscard]] ReservationListResult find_by_creator(
        const haven::domain::OrganizationId&, const haven::domain::UserId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
        const haven::domain::OrganizationId&) const override {
        return reservations_;
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

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const haven::domain::OrganizationId&, const haven::domain::Reservation&) override {
        return haven::application::persistence::PersistenceToken{1};
    }
    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const haven::domain::OrganizationId&,
        const haven::domain::Reservation&,
        const haven::application::persistence::PersistenceToken&) override {
        return haven::application::persistence::PersistenceToken{2};
    }

private:
    ReservationListResult reservations_;
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] haven::domain::Reservation make_pending_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::UserId& creator_id) {
    return haven::domain::Reservation::create_pending_approval(
        organization_id,
        reservation_id,
        resource_id,
        creator_id,
        haven::domain::TimeInterval{make_time_point(10), make_time_point(11)},
        haven::domain::Purpose{"Planning meeting"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created-" + reservation_id.value()},
        haven::domain::EventId{"event-approval-requested-" + reservation_id.value()},
        make_time_point(9));
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
        haven::domain::EventId{"event-created-" + reservation_id.value()},
        haven::domain::EventId{"event-confirmed-" + reservation_id.value()},
        make_time_point(9));
}

TEST(ListPendingApprovalsHandlerTest, Handle_ShouldReturnPendingReservations_WhenMatchesExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_pending_reservation(haven::domain::ReservationId{"reservation-100"},
                                            organization_id,
                                            haven::domain::ResourceId{"resource-executive-room"},
                                            haven::domain::UserId{"user-100"}));
    repository.add(make_pending_reservation(haven::domain::ReservationId{"reservation-101"},
                                            organization_id,
                                            haven::domain::ResourceId{"resource-training-room"},
                                            haven::domain::UserId{"user-200"}));
    const auto handler = ListPendingApprovalsHandler{repository};

    const auto result = handler.handle(ListPendingApprovalsQuery{organization_id});

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result.at(0).organization_id(), organization_id);
    EXPECT_EQ(result.at(0).status(), haven::domain::ReservationStatus::PendingApproval);
    EXPECT_EQ(result.at(1).organization_id(), organization_id);
    EXPECT_EQ(result.at(1).status(), haven::domain::ReservationStatus::PendingApproval);
}

TEST(ListPendingApprovalsHandlerTest, Handle_ShouldReturnEmpty_WhenNoPendingApprovalsExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    const auto handler = ListPendingApprovalsHandler{repository};

    const auto result = handler.handle(ListPendingApprovalsQuery{organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListPendingApprovalsHandlerTest,
     Handle_ShouldExcludePendingReservationsFromAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_pending_reservation(haven::domain::ReservationId{"reservation-100"},
                                            owner_organization_id,
                                            haven::domain::ResourceId{"resource-executive-room"},
                                            haven::domain::UserId{"user-100"}));
    const auto handler = ListPendingApprovalsHandler{repository};

    const auto result = handler.handle(ListPendingApprovalsQuery{caller_organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListPendingApprovalsHandlerTest, Handle_ShouldExcludeConfirmedReservations) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_confirmed_reservation(haven::domain::ReservationId{"reservation-100"},
                                              organization_id,
                                              haven::domain::ResourceId{"resource-boardroom"},
                                              haven::domain::UserId{"user-100"}));
    const auto handler = ListPendingApprovalsHandler{repository};

    const auto result = handler.handle(ListPendingApprovalsQuery{organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListPendingApprovalsHandlerTest,
     Handle_ShouldRemoveEntriesOutsideApprovalScope_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = ApprovalQueueLeakingRepository{ReservationListResult{
        make_pending_reservation(haven::domain::ReservationId{"reservation-100"},
                                 organization_id,
                                 haven::domain::ResourceId{"resource-executive-room"},
                                 haven::domain::UserId{"user-100"}),
        make_pending_reservation(haven::domain::ReservationId{"reservation-101"},
                                 haven::domain::OrganizationId{"organization-beta"},
                                 haven::domain::ResourceId{"resource-training-room"},
                                 haven::domain::UserId{"user-200"}),
        make_confirmed_reservation(haven::domain::ReservationId{"reservation-102"},
                                   organization_id,
                                   haven::domain::ResourceId{"resource-boardroom"},
                                   haven::domain::UserId{"user-300"})}};
    const auto handler = ListPendingApprovalsHandler{repository};

    const auto result = handler.handle(ListPendingApprovalsQuery{organization_id});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().organization_id(), organization_id);
    EXPECT_EQ(result.front().status(), haven::domain::ReservationStatus::PendingApproval);
}

}  // namespace
}  // namespace haven::application::reservations
