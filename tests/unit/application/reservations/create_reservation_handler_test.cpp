/**
 * @file create_reservation_handler_test.cpp
 * @brief Tests CreateReservation application orchestration.
 */

#include "haven/application/reservations/create_reservation_handler.hpp"

#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/policies/reservation_creation_policy.hpp"
#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
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

using haven::application::resources::ResourceLookupResult;
using haven::application::resources::ResourceRepository;
using haven::application::resources::ResourceSearchResult;
using haven::domain::EventId;
using haven::domain::OrganizationId;
using haven::domain::Purpose;
using haven::domain::Reservation;
using haven::domain::ReservationCreationPolicy;
using haven::domain::ReservationId;
using haven::domain::ReservationKind;
using haven::domain::ReservationStatus;
using haven::domain::Resource;
using haven::domain::ResourceId;
using haven::domain::ResourceType;
using haven::domain::TimeInterval;
using haven::domain::UserId;

using namespace std::chrono_literals;

class InMemoryResourceRepository final : public ResourceRepository {
public:
    void add(Resource resource) {
        resources_.push_back(std::move(resource));
    }

    [[nodiscard]] ResourceLookupResult find_by_id(const OrganizationId& organization_id,
                                                  const ResourceId& resource_id) const override {
        const auto resource =
            std::find_if(resources_.cbegin(),
                         resources_.cend(),
                         [&organization_id, &resource_id](const Resource& candidate) {
                             return candidate.organization_id() == organization_id &&
                                    candidate.resource_id() == resource_id;
                         });

        if (resource == resources_.cend()) {
            return std::nullopt;
        }

        return haven::application::resources::LoadedResource{
            *resource, haven::application::persistence::PersistenceToken{1}};
    }

    [[nodiscard]] ResourceSearchResult find_active_by_type(const OrganizationId&,
                                                           ResourceType) const override {
        return {};
    }

private:
    std::vector<Resource> resources_;
};

class InMemoryReservationRepository final : public ReservationRepository {
public:
    void set_conflict(const bool conflict) noexcept {
        conflict_ = conflict;
    }

    [[nodiscard]] bool has_conflict(const OrganizationId&,
                                    const ResourceId&,
                                    const TimeInterval&) const override {
        return conflict_;
    }

    [[nodiscard]] bool has_conflict_excluding(const OrganizationId&,
                                              const ResourceId&,
                                              const TimeInterval&,
                                              const ReservationId&) const override {
        return false;
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken insert(
        const OrganizationId& organization_id, const Reservation& reservation) override {
        saved_organization_id_ = organization_id;
        saved_reservation_ = reservation;
        return haven::application::persistence::PersistenceToken{1};
    }

    [[nodiscard]] haven::application::persistence::PersistenceToken update(
        const OrganizationId&,
        const Reservation&,
        const haven::application::persistence::PersistenceToken&) override {
        return haven::application::persistence::PersistenceToken{2};
    }

    [[nodiscard]] const std::optional<OrganizationId>& saved_organization_id() const noexcept {
        return saved_organization_id_;
    }

    [[nodiscard]] const std::optional<Reservation>& saved_reservation() const noexcept {
        return saved_reservation_;
    }

    [[nodiscard]] ReservationLookupResult find_by_id(const OrganizationId&,
                                                     const ReservationId&) const override {
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

    [[nodiscard]] ReservationListResult find_by_resource_and_interval(
        const OrganizationId&, const ResourceId&, const TimeInterval&) const override {
        return {};
    }

private:
    bool conflict_{false};
    std::optional<OrganizationId> saved_organization_id_;
    std::optional<Reservation> saved_reservation_;
};

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] TimeInterval make_interval() {
    return TimeInterval{make_time_point(10), make_time_point(11)};
}

[[nodiscard]] Resource make_resource(const ResourceId& resource_id,
                                     const OrganizationId& organization_id,
                                     const bool requires_approval) {
    return Resource{organization_id,
                    resource_id,
                    ResourceType::MeetingRoom,
                    haven::domain::ResourceStatus::Active,
                    requires_approval};
}

[[nodiscard]] CreateReservationCommand make_command(
    const OrganizationId& organization_id,
    const ResourceId& resource_id,
    const ReservationKind reservation_kind = ReservationKind::Standard,
    const bool maintenance_authorized = false) {
    return CreateReservationCommand{organization_id,
                                    ReservationId{"reservation-100"},
                                    resource_id,
                                    UserId{"user-100"},
                                    make_interval(),
                                    Purpose{"Planning meeting"},
                                    reservation_kind,
                                    maintenance_authorized,
                                    EventId{"event-created-100"},
                                    EventId{"event-confirmed-100"},
                                    EventId{"event-approval-100"},
                                    make_time_point(9)};
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldCreateConfirmedReservation_WhenResourceDoesNotRequireApproval) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    const auto creation_policy = ReservationCreationPolicy{};
    resource_repository.add(make_resource(resource_id, organization_id, false));
    const auto handler =
        CreateReservationHandler{resource_repository, reservation_repository, creation_policy};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    ASSERT_EQ(result.status(), CreateReservationStatus::CREATED_CONFIRMED);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->status(), ReservationStatus::Confirmed);
    ASSERT_TRUE(reservation_repository.saved_reservation().has_value());
    EXPECT_EQ(reservation_repository.saved_organization_id(), organization_id);
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldCreatePendingReservation_WhenResourceRequiresApproval) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-executive-room"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    const auto creation_policy = ReservationCreationPolicy{};
    resource_repository.add(make_resource(resource_id, organization_id, true));
    const auto handler =
        CreateReservationHandler{resource_repository, reservation_repository, creation_policy};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    ASSERT_EQ(result.status(), CreateReservationStatus::CREATED_PENDING_APPROVAL);
    ASSERT_TRUE(result.reservation().has_value());
    EXPECT_EQ(result.reservation()->status(), ReservationStatus::PendingApproval);
    EXPECT_TRUE(reservation_repository.saved_reservation().has_value());
}

TEST(CreateReservationHandlerTest, Handle_ShouldReturnNotFound_WhenResourceDoesNotExist) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-missing"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    const auto creation_policy = ReservationCreationPolicy{};
    const auto handler =
        CreateReservationHandler{resource_repository, reservation_repository, creation_policy};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    EXPECT_EQ(result.status(), CreateReservationStatus::RESOURCE_NOT_FOUND);
    EXPECT_FALSE(result.reservation().has_value());
    EXPECT_FALSE(reservation_repository.saved_reservation().has_value());
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldReturnNotFound_WhenResourceBelongsToAnotherOrganization) {
    const auto caller_organization_id = OrganizationId{"organization-alpha"};
    const auto owner_organization_id = OrganizationId{"organization-beta"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    const auto creation_policy = ReservationCreationPolicy{};
    resource_repository.add(make_resource(resource_id, owner_organization_id, false));
    const auto handler =
        CreateReservationHandler{resource_repository, reservation_repository, creation_policy};

    const auto result = handler.handle(make_command(caller_organization_id, resource_id));

    EXPECT_EQ(result.status(), CreateReservationStatus::RESOURCE_NOT_FOUND);
    EXPECT_FALSE(reservation_repository.saved_reservation().has_value());
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldReturnConflict_WhenConfirmedReservationOverlapsInterval) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    const auto creation_policy = ReservationCreationPolicy{};
    resource_repository.add(make_resource(resource_id, organization_id, false));
    reservation_repository.set_conflict(true);
    const auto handler =
        CreateReservationHandler{resource_repository, reservation_repository, creation_policy};

    const auto result = handler.handle(make_command(organization_id, resource_id));

    EXPECT_EQ(result.status(), CreateReservationStatus::SCHEDULE_CONFLICT);
    EXPECT_FALSE(reservation_repository.saved_reservation().has_value());
}

TEST(CreateReservationHandlerTest,
     Handle_ShouldRejectStandardReservation_WhenDurationExceedsTwelveHours) {
    const auto organization_id = OrganizationId{"organization-alpha"};
    const auto resource_id = ResourceId{"resource-boardroom"};
    auto resource_repository = InMemoryResourceRepository{};
    auto reservation_repository = InMemoryReservationRepository{};
    const auto creation_policy = ReservationCreationPolicy{};
    resource_repository.add(make_resource(resource_id, organization_id, false));
    const auto handler =
        CreateReservationHandler{resource_repository, reservation_repository, creation_policy};
    const auto command =
        CreateReservationCommand{organization_id,
                                 ReservationId{"reservation-100"},
                                 resource_id,
                                 UserId{"user-100"},
                                 TimeInterval{make_time_point(10), make_time_point(23)},
                                 Purpose{"Planning meeting"},
                                 ReservationKind::Standard,
                                 false,
                                 EventId{"event-created-100"},
                                 EventId{"event-confirmed-100"},
                                 EventId{"event-approval-100"},
                                 make_time_point(9)};

    const auto result = handler.handle(command);

    EXPECT_EQ(result.status(), CreateReservationStatus::POLICY_REJECTED);
    EXPECT_FALSE(reservation_repository.saved_reservation().has_value());
}

}  // namespace
}  // namespace haven::application::reservations
