/**
 * @file list_decided_approvals_handler_test.cpp
 * @brief Tests ListDecidedApprovals application orchestration.
 */

#include "haven/application/reservations/list_decided_approvals_handler.hpp"

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
        const haven::domain::OrganizationId&, const haven::domain::ReservationId&) const override {
        return std::nullopt;
    }

    [[nodiscard]] ReservationListResult find_by_creator(
        const haven::domain::OrganizationId&, const haven::domain::UserId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_pending_approvals(
        const haven::domain::OrganizationId&) const override {
        return {};
    }

    [[nodiscard]] ReservationListResult find_decided_approvals(
        const haven::domain::OrganizationId& organization_id) const override {
        auto result = ReservationListResult{};

        for (const auto& reservation : reservations_) {
            if (reservation.organization_id() == organization_id) {
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

class DecidedQueueLeakingRepository final : public ReservationRepository {
public:
    explicit DecidedQueueLeakingRepository(ReservationListResult reservations)
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
        return {};
    }

    [[nodiscard]] ReservationListResult find_decided_approvals(
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

[[nodiscard]] haven::domain::Reservation make_approved_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::UserId& creator_id) {
    auto reservation =
        make_pending_reservation(reservation_id, organization_id, resource_id, creator_id);
    reservation.approve(haven::domain::UserId{"approver-100"},
                        make_time_point(12),
                        haven::domain::EventId{"event-confirmed-" + reservation_id.value()});
    return reservation;
}

[[nodiscard]] haven::domain::Reservation make_rejected_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::UserId& creator_id) {
    auto reservation =
        make_pending_reservation(reservation_id, organization_id, resource_id, creator_id);
    reservation.reject(haven::domain::UserId{"approver-100"},
                       make_time_point(12),
                       haven::domain::EventId{"event-rejected-" + reservation_id.value()});
    return reservation;
}

TEST(ListDecidedApprovalsHandlerTest, Handle_ShouldReturnApprovedAndRejectedReservations) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_approved_reservation(haven::domain::ReservationId{"reservation-100"},
                                             organization_id,
                                             haven::domain::ResourceId{"resource-executive-room"},
                                             haven::domain::UserId{"user-100"}));
    repository.add(make_rejected_reservation(haven::domain::ReservationId{"reservation-101"},
                                             organization_id,
                                             haven::domain::ResourceId{"resource-training-room"},
                                             haven::domain::UserId{"user-200"}));
    const auto handler = ListDecidedApprovalsHandler{repository};

    const auto result = handler.handle(ListDecidedApprovalsQuery{organization_id});

    ASSERT_EQ(result.size(), 2U);
}

TEST(ListDecidedApprovalsHandlerTest, Handle_ShouldExcludeStillPendingReservations) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_pending_reservation(haven::domain::ReservationId{"reservation-100"},
                                            organization_id,
                                            haven::domain::ResourceId{"resource-executive-room"},
                                            haven::domain::UserId{"user-100"}));
    const auto handler = ListDecidedApprovalsHandler{repository};

    const auto result = handler.handle(ListDecidedApprovalsQuery{organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListDecidedApprovalsHandlerTest, Handle_ShouldExcludeDirectlyConfirmedReservations) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_confirmed_reservation(haven::domain::ReservationId{"reservation-100"},
                                              organization_id,
                                              haven::domain::ResourceId{"resource-boardroom"},
                                              haven::domain::UserId{"user-100"}));
    const auto handler = ListDecidedApprovalsHandler{repository};

    const auto result = handler.handle(ListDecidedApprovalsQuery{organization_id});

    EXPECT_TRUE(result.empty());
}

TEST(ListDecidedApprovalsHandlerTest,
     Handle_ShouldRemoveEntriesOutsideDecisionScope_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto repository = DecidedQueueLeakingRepository{ReservationListResult{
        make_rejected_reservation(haven::domain::ReservationId{"reservation-100"},
                                  organization_id,
                                  haven::domain::ResourceId{"resource-executive-room"},
                                  haven::domain::UserId{"user-100"}),
        make_rejected_reservation(haven::domain::ReservationId{"reservation-101"},
                                  haven::domain::OrganizationId{"organization-beta"},
                                  haven::domain::ResourceId{"resource-training-room"},
                                  haven::domain::UserId{"user-200"}),
        make_pending_reservation(haven::domain::ReservationId{"reservation-102"},
                                 organization_id,
                                 haven::domain::ResourceId{"resource-boardroom"},
                                 haven::domain::UserId{"user-300"})}};
    const auto handler = ListDecidedApprovalsHandler{repository};

    const auto result = handler.handle(ListDecidedApprovalsQuery{organization_id});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().organization_id(), organization_id);
    EXPECT_EQ(result.front().status(), haven::domain::ReservationStatus::Rejected);
}

}  // namespace
}  // namespace haven::application::reservations
