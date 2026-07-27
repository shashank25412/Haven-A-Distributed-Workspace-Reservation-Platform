/**
 * @file get_resource_calendar_handler_test.cpp
 * @brief Tests GetResourceCalendar application orchestration.
 */

#include "haven/application/resources/get_resource_calendar_handler.hpp"

#include "haven/domain/reservation.hpp"
#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include "application/util/test_reservation_repository.hpp"
#include "application/util/test_resource_repository.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace haven::application::resources {
namespace {

using haven::tests::util::application::TestReservationRepository;
using haven::tests::util::application::TestResourceRepository;

[[nodiscard]] auto make_time_point(const int hour) {
    return std::chrono::system_clock::time_point{} + std::chrono::hours{hour};
}

[[nodiscard]] haven::domain::TimeInterval make_calendar_interval() {
    return haven::domain::TimeInterval{make_time_point(9), make_time_point(17)};
}

[[nodiscard]] haven::domain::Reservation make_reservation(
    const haven::domain::ReservationId& reservation_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id,
    const haven::domain::TimeInterval& interval) {
    return haven::domain::Reservation::create_confirmed(
        organization_id,
        reservation_id,
        resource_id,
        haven::domain::UserId{"user-100"},
        interval,
        haven::domain::Purpose{"Calendar entry"},
        haven::domain::ReservationKind::Standard,
        haven::domain::EventId{"event-created-" + reservation_id.value()},
        haven::domain::EventId{"event-confirmed-" + reservation_id.value()},
        make_time_point(8));
}

[[nodiscard]] haven::domain::Resource make_resource(
    const haven::domain::ResourceId& resource_id,
    const haven::domain::OrganizationId& organization_id) {
    return haven::domain::Resource{organization_id,
                                   resource_id,
                                   haven::domain::ResourceType::MeetingRoom,
                                   haven::domain::ResourceStatus::Active,
                                   false};
}

TEST(GetResourceCalendarHandlerTest, Handle_ShouldReturnCalendar_WhenResourceExists) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    resource_repository.set_lookup_result(make_resource(resource_id, organization_id));
    reservation_repository.set_calendar_result(
        {make_reservation(haven::domain::ReservationId{"reservation-100"},
                          organization_id,
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(10), make_time_point(11)}),
         make_reservation(haven::domain::ReservationId{"reservation-101"},
                          organization_id,
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(14), make_time_point(15)})});
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{organization_id, resource_id, make_calendar_interval()});

    ASSERT_EQ(result.status(), GetResourceCalendarStatus::FOUND);
    ASSERT_EQ(result.reservations().size(), 2U);
    EXPECT_EQ(result.reservations().at(0).resource_id(), resource_id);
    EXPECT_EQ(result.reservations().at(1).resource_id(), resource_id);
}

TEST(GetResourceCalendarHandlerTest, Handle_ShouldReturnEmptyCalendar_WhenNoReservationsOverlap) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    resource_repository.set_lookup_result(make_resource(resource_id, organization_id));
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{organization_id, resource_id, make_calendar_interval()});

    EXPECT_EQ(result.status(), GetResourceCalendarStatus::FOUND);
    EXPECT_TRUE(result.reservations().empty());
}

TEST(GetResourceCalendarHandlerTest, Handle_ShouldReturnNotFound_WhenResourceDoesNotExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(GetResourceCalendarQuery{
        organization_id, haven::domain::ResourceId{"resource-missing"}, make_calendar_interval()});

    EXPECT_EQ(result.status(), GetResourceCalendarStatus::RESOURCE_NOT_FOUND);
    EXPECT_TRUE(result.reservations().empty());
}

TEST(GetResourceCalendarHandlerTest,
     Handle_ShouldReturnNotFound_WhenResourceBelongsToAnotherOrganization) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{caller_organization_id, resource_id, make_calendar_interval()});

    EXPECT_EQ(result.status(), GetResourceCalendarStatus::RESOURCE_NOT_FOUND);
    EXPECT_TRUE(result.reservations().empty());
}

TEST(GetResourceCalendarHandlerTest,
     Handle_ShouldReturnNotFound_WhenResourceRepositoryLeaksTenant) {
    const auto caller_organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto owner_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_lookup_result(make_resource(resource_id, owner_organization_id));
    auto reservation_repository = TestReservationRepository{};
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{caller_organization_id, resource_id, make_calendar_interval()});

    EXPECT_EQ(result.status(), GetResourceCalendarStatus::RESOURCE_NOT_FOUND);
    EXPECT_TRUE(result.reservations().empty());
}

TEST(GetResourceCalendarHandlerTest,
     Handle_ShouldRemoveReservationsFromAnotherOrganization_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_lookup_result(make_resource(resource_id, organization_id));
    auto reservation_repository = TestReservationRepository{};
    reservation_repository.set_calendar_result(
        {make_reservation(haven::domain::ReservationId{"reservation-100"},
                          organization_id,
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(10), make_time_point(11)}),
         make_reservation(haven::domain::ReservationId{"reservation-101"},
                          haven::domain::OrganizationId{"organization-beta"},
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(12), make_time_point(13)})});
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{organization_id, resource_id, make_calendar_interval()});

    ASSERT_EQ(result.status(), GetResourceCalendarStatus::FOUND);
    ASSERT_EQ(result.reservations().size(), 1U);
    EXPECT_EQ(result.reservations().front().organization_id(), organization_id);
}

TEST(GetResourceCalendarHandlerTest,
     Handle_ShouldRemoveReservationsForAnotherResource_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_lookup_result(make_resource(resource_id, organization_id));
    auto reservation_repository = TestReservationRepository{};
    reservation_repository.set_calendar_result(
        {make_reservation(haven::domain::ReservationId{"reservation-100"},
                          organization_id,
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(10), make_time_point(11)}),
         make_reservation(haven::domain::ReservationId{"reservation-101"},
                          organization_id,
                          haven::domain::ResourceId{"resource-room-200"},
                          haven::domain::TimeInterval{make_time_point(12), make_time_point(13)})});
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{organization_id, resource_id, make_calendar_interval()});

    ASSERT_EQ(result.status(), GetResourceCalendarStatus::FOUND);
    ASSERT_EQ(result.reservations().size(), 1U);
    EXPECT_EQ(result.reservations().front().resource_id(), resource_id);
}

TEST(GetResourceCalendarHandlerTest,
     Handle_ShouldRemoveNonOverlappingReservations_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_lookup_result(make_resource(resource_id, organization_id));
    auto reservation_repository = TestReservationRepository{};
    reservation_repository.set_calendar_result(
        {make_reservation(haven::domain::ReservationId{"reservation-100"},
                          organization_id,
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(10), make_time_point(11)}),
         make_reservation(haven::domain::ReservationId{"reservation-101"},
                          organization_id,
                          resource_id,
                          haven::domain::TimeInterval{make_time_point(17), make_time_point(18)})});
    const auto handler = GetResourceCalendarHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(
        GetResourceCalendarQuery{organization_id, resource_id, make_calendar_interval()});

    ASSERT_EQ(result.status(), GetResourceCalendarStatus::FOUND);
    ASSERT_EQ(result.reservations().size(), 1U);
    EXPECT_EQ(result.reservations().front().reservation_id(),
              haven::domain::ReservationId{"reservation-100"});
}

}  // namespace
}  // namespace haven::application::resources
