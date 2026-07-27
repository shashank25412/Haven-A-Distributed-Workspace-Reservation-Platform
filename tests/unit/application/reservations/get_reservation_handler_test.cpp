/**
 * @file get_reservation_handler_test.cpp
 * @brief Tests tenant-safe GetReservation application orchestration.
 */

#include "haven/application/reservations/get_reservation_handler.hpp"
#include "haven/domain/reservation.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
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
    void add(
        haven::domain::OrganizationId organization_id,
        haven::domain::ReservationId reservation_id,
        haven::domain::Reservation reservation) {
        reservations_.push_back(StoredReservation{
            std::move(organization_id),
            std::move(reservation_id),
            std::move(reservation)});
    }

    [[nodiscard]] ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ReservationId& reservation_id) const override {
        const auto reservation = std::find_if(
            reservations_.cbegin(),
            reservations_.cend(),
            [&organization_id, &reservation_id](
                const StoredReservation& stored_reservation) {
                return stored_reservation.organization_id == organization_id
                    && stored_reservation.reservation_id == reservation_id;
            });

        if (reservation == reservations_.cend()) {
            return std::nullopt;
        }

        return reservation->reservation;
    }

    [[nodiscard]] ReservationListResult find_by_creator(
        const haven::domain::OrganizationId&,
        const haven::domain::UserId&) const override {
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

    [[nodiscard]] bool has_conflict(
        const haven::domain::OrganizationId&,
        const haven::domain::ResourceId&,
        const haven::domain::TimeInterval&) const override {
        return false;
    }

    [[nodiscard]] bool has_conflict_excluding(
        const haven::domain::OrganizationId&,
        const haven::domain::ResourceId&,
        const haven::domain::TimeInterval&,
        const haven::domain::ReservationId&) const override {
        return false;
    }

    void save(
        const haven::domain::OrganizationId&,
        const haven::domain::Reservation&) override {}

private:
    struct StoredReservation final {
        haven::domain::OrganizationId organization_id;
        haven::domain::ReservationId reservation_id;
        haven::domain::Reservation reservation;
    };

    std::vector<StoredReservation> reservations_;
};

class TenantLeakingReservationRepository final : public ReservationRepository {
public:
    explicit TenantLeakingReservationRepository(haven::domain::Reservation reservation)
        : reservation_(std::move(reservation)) {}

    [[nodiscard]] ReservationLookupResult find_by_id(
        const haven::domain::OrganizationId&,
        const haven::domain::ReservationId&) const override {
        return reservation_;
    }

    [[nodiscard]] ReservationListResult find_by_creator(
        const haven::domain::OrganizationId&,
        const haven::domain::UserId&) const override {
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

    [[nodiscard]] bool has_conflict(
        const haven::domain::OrganizationId&,
        const haven::domain::ResourceId&,
        const haven::domain::TimeInterval&) const override {
        return false;
    }

    [[nodiscard]] bool has_conflict_excluding(
        const haven::domain::OrganizationId&,
        const haven::domain::ResourceId&,
        const haven::domain::TimeInterval&,
        const haven::domain::ReservationId&) const override {
        return false;
    }

    void save(
        const haven::domain::OrganizationId&,
        const haven::domain::Reservation&) override {}

private:
    haven::domain::Reservation reservation_;
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] haven::domain::Reservation make_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) {
    return haven::domain::Reservation::create_confirmed(
        organization_id,
        reservation_id,
        resource_id,
        haven::domain::UserId{"user-100"},
        haven::domain::TimeInterval{make_time_point(10), make_time_point(11)},
        haven::domain::Purpose{"Planning meeting"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created-100"},
        haven::domain::EventId{"event-confirmed-100"},
        make_time_point(9));
}

TEST(GetReservationHandlerTest, Handle_ShouldReturnReservation_WhenReservationBelongsToOrganization) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = InMemoryReservationRepository{};
    repository.add(
        organization_id,
        reservation_id,
        make_reservation(reservation_id, organization_id, resource_id));
    const auto handler = GetReservationHandler{repository};

    const auto result = handler.handle(
        GetReservationQuery{organization_id, reservation_id});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->organization_id(), organization_id);
}

TEST(GetReservationHandlerTest, Handle_ShouldReturnEmpty_WhenReservationDoesNotExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-missing"};
    auto repository = InMemoryReservationRepository{};
    const auto handler = GetReservationHandler{repository};

    const auto result = handler.handle(
        GetReservationQuery{organization_id, reservation_id});

    EXPECT_FALSE(result.has_value());
}

TEST(GetReservationHandlerTest, Handle_ShouldReturnEmpty_WhenReservationBelongsToAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = InMemoryReservationRepository{};
    repository.add(
        owner_organization_id,
        reservation_id,
        make_reservation(
            reservation_id,
            owner_organization_id,
            resource_id));
    const auto handler = GetReservationHandler{repository};

    const auto result = handler.handle(
        GetReservationQuery{
            caller_organization_id,
            reservation_id});

    EXPECT_FALSE(result.has_value());
}

TEST(GetReservationHandlerTest, Handle_ShouldReturnEmpty_WhenRepositoryReturnsCrossTenantReservation) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto reservation_id = haven::domain::ReservationId{"reservation-100"};
    const auto resource_id = haven::domain::ResourceId{"resource-boardroom"};
    auto repository = TenantLeakingReservationRepository{
        make_reservation(
            reservation_id,
            owner_organization_id,
            resource_id)};
    const auto handler = GetReservationHandler{repository};

    const auto result = handler.handle(
        GetReservationQuery{
            caller_organization_id,
            reservation_id});

    EXPECT_FALSE(result.has_value());
}

}  // namespace
}  // namespace haven::application::reservations
