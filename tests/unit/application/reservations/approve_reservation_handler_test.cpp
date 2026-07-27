/**
 * @file approve_reservation_handler_test.cpp
 * @brief Tests ApproveReservation application orchestration.
 */

#include "haven/application/reservations/approve_reservation_handler.hpp"

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

    void set_conflict(const bool conflict) noexcept {
        conflict_ = conflict;
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

        return *reservation;
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
        return conflict_;
    }

    [[nodiscard]] bool has_conflict_excluding(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id,
        const haven::domain::TimeInterval& interval,
        const haven::domain::ReservationId& excluded_reservation_id) const override {
        checked_organization_id_ = organization_id;
        checked_resource_id_ = resource_id;
        checked_interval_ = interval;
        excluded_reservation_id_ = excluded_reservation_id;
        return conflict_;
    }

    void save(const haven::domain::OrganizationId& organization_id,
              const haven::domain::Reservation& reservation) override {
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
    }

    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& checked_organization_id()
        const noexcept {
        return checked_organization_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::ResourceId>& checked_resource_id()
        const noexcept {
        return checked_resource_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::TimeInterval>& checked_interval()
        const noexcept {
        return checked_interval_;
    }

    [[nodiscard]] const std::optional<haven::domain::ReservationId>& excluded_reservation_id()
        const noexcept {
        return excluded_reservation_id_;
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
    bool conflict_{false};
    mutable std::optional<haven::domain::OrganizationId> checked_organization_id_;
    mutable std::optional<haven::domain::ResourceId> checked_resource_id_;
    mutable std::optional<haven::domain::TimeInterval> checked_interval_;
    mutable std::optional<haven::domain::ReservationId> excluded_reservation_id_;
    std::optional<haven::domain::OrganizationId> saved_organization_id_;
    std::optional<haven::domain::Reservation> saved_reservation_;
};

class TenantLeakingReservationRepository final : public ReservationRepository {
public:
    explicit TenantLeakingReservationRepository(haven::domain::Reservation reservation)
        : reservation_(std::move(reservation)) {}

    [[nodiscard]] ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId&, const haven::domain::ReservationId&) const override {
        return reservation_;
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

    void save(const haven::domain::OrganizationId&, const haven::domain::Reservation&) override {
        save_called_ = true;
    }

    [[nodiscard]] bool save_called() const noexcept {
        return save_called_;
    }

private:
    haven::domain::Reservation reservation_;
    bool save_called_{false};
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] haven::domain::TimeInterval make_interval() {
    return haven::domain::TimeInterval{make_time_point(10), make_time_point(11)};
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
        make_interval(),
        haven::domain::Purpose{"Executive planning meeting"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created-100"},
        haven::domain::EventId{"event-approval-requested-100"},
        make_time_point(9));
}

[[nodiscard]] haven::domain::Reservation make_confirmed_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::UserId& creator_id) {
    return haven::domain::Reservation::create_confirmed(organization_id,
                                                        reservation_id,
                                                        resource_id,
                                                        creator_id,
                                                        make_interval(),
                                                        haven::domain::Purpose{"Planning meeting"},
                                                        haven::domain::ReservationKind::Standard,
                                                        haven::domain::EventId{"event-created-100"},
                                                        haven::domain::EventId{"event-confirmed-100"},
                                                        make_time_point(9));
}

[[nodiscard]] ApproveReservationCommand make_command(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::UserId& approver_id) {
    return ApproveReservationCommand{organization_id,
                                     reservation_id,
                                     approver_id,
                                     haven::domain::EventId{"event-approved-100"},
                                     make_time_point(9)};
}

TEST(ApproveReservationHandlerTest, Handle_ShouldApproveReservation_WhenRequestIsValid) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto resource_id = haven::domain::ResourceId{"resource-executive-room"};
    const auto approver_id = haven::domain::UserId{"approver-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_pending_reservation(
        reservation_id, organization_id, resource_id, haven::domain::UserId{"user-100"}));
    const auto handler = ApproveReservationHandler{repository};

    const auto result = handler.handle(make_command(organization_id, reservation_id, approver_id));

    ASSERT_EQ(result.status(), ApproveReservationStatus::APPROVED);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->status(), haven::domain::ReservationStatus::Confirmed);
    ASSERT_TRUE(repository.saved_reservation().has_value());
    EXPECT_EQ(repository.saved_reservation()->status(),
              haven::domain::ReservationStatus::Confirmed);
    EXPECT_EQ(repository.saved_organization_id(), organization_id);
    EXPECT_EQ(repository.checked_organization_id(), organization_id);
    EXPECT_EQ(repository.checked_resource_id(), resource_id);
    EXPECT_EQ(repository.checked_interval(), make_interval());
    EXPECT_EQ(repository.excluded_reservation_id(), reservation_id);
}

TEST(ApproveReservationHandlerTest, Handle_ShouldReturnNotFound_WhenReservationDoesNotExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-missing"};
    auto repository = InMemoryReservationRepository{};
    const auto handler = ApproveReservationHandler{repository};

    const auto result = handler.handle(
        make_command(organization_id, reservation_id, haven::domain::UserId{"approver-100"}));

    EXPECT_EQ(result.status(), ApproveReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ApproveReservationHandlerTest,
     Handle_ShouldReturnNotFound_WhenReservationBelongsToAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_pending_reservation(reservation_id,
                                            owner_organization_id,
                                            haven::domain::ResourceId{"resource-executive-room"},
                                            haven::domain::UserId{"user-100"}));
    const auto handler = ApproveReservationHandler{repository};

    const auto result = handler.handle(make_command(
        caller_organization_id, reservation_id, haven::domain::UserId{"approver-100"}));

    EXPECT_EQ(result.status(), ApproveReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ApproveReservationHandlerTest,
     Handle_ShouldReturnNotFound_WhenRepositoryLeaksCrossTenantReservation) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    auto repository = TenantLeakingReservationRepository{
        make_pending_reservation(reservation_id,
                                 owner_organization_id,
                                 haven::domain::ResourceId{"resource-executive-room"},
                                 haven::domain::UserId{"user-100"})};
    const auto handler = ApproveReservationHandler{repository};

    const auto result = handler.handle(make_command(
        caller_organization_id, reservation_id, haven::domain::UserId{"approver-100"}));

    EXPECT_EQ(result.status(), ApproveReservationStatus::RESERVATION_NOT_FOUND);
    EXPECT_FALSE(repository.save_called());
}

TEST(ApproveReservationHandlerTest, Handle_ShouldRejectApproval_WhenReservationIsAlreadyConfirmed) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_confirmed_reservation(reservation_id,
                                              organization_id,
                                              haven::domain::ResourceId{"resource-boardroom"},
                                              haven::domain::UserId{"user-100"}));
    const auto handler = ApproveReservationHandler{repository};

    const auto result = handler.handle(
        make_command(organization_id, reservation_id, haven::domain::UserId{"approver-100"}));

    EXPECT_EQ(result.status(), ApproveReservationStatus::INVALID_STATE);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
}

TEST(ApproveReservationHandlerTest, Handle_ShouldRejectApproval_WhenScheduleConflicts) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    auto repository = InMemoryReservationRepository{};
    repository.add(make_pending_reservation(reservation_id,
                                            organization_id,
                                            haven::domain::ResourceId{"resource-executive-room"},
                                            haven::domain::UserId{"user-100"}));
    repository.set_conflict(true);
    const auto handler = ApproveReservationHandler{repository};

    const auto result = handler.handle(
        make_command(organization_id, reservation_id, haven::domain::UserId{"approver-100"}));

    EXPECT_EQ(result.status(), ApproveReservationStatus::SCHEDULE_CONFLICT);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(repository.saved_reservation().has_value());
    EXPECT_EQ(repository.excluded_reservation_id(), reservation_id);
}

}  // namespace
}  // namespace haven::application::reservations
