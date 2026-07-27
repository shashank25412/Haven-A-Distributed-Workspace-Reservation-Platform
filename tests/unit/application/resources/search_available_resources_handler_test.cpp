/**
 * @file search_available_resources_handler_test.cpp
 * @brief Tests SearchAvailableResources application orchestration.
 */

#include "haven/application/resources/search_available_resources_handler.hpp"

#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/time_interval.hpp"

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

[[nodiscard]] haven::domain::TimeInterval make_interval() {
    return haven::domain::TimeInterval{make_time_point(10), make_time_point(11)};
}

[[nodiscard]] haven::domain::Resource make_resource(
    const haven::domain::ResourceId& resource_id,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceType resource_type,
    const haven::domain::ResourceStatus resource_status = haven::domain::ResourceStatus::Active) {
    return haven::domain::Resource{
        organization_id, resource_id, resource_type, resource_status, false};
}

TEST(SearchAvailableResourcesHandlerTest,
     Handle_ShouldReturnAvailableResources_WhenNoConflictsExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto first_resource_id = haven::domain::ResourceId{"resource-room-100"};
    const auto second_resource_id = haven::domain::ResourceId{"resource-room-200"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    resource_repository.set_search_result(
        {make_resource(
             first_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom),
         make_resource(
             second_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom)});
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result.at(0).resource_id(), first_resource_id);
    EXPECT_EQ(result.at(1).resource_id(), second_resource_id);
    ASSERT_EQ(reservation_repository.conflict_resource_ids().size(), 2U);
    EXPECT_EQ(reservation_repository.conflict_organization_ids().at(0), organization_id);
    EXPECT_EQ(reservation_repository.conflict_organization_ids().at(1), organization_id);
    EXPECT_EQ(reservation_repository.conflict_intervals().at(0), make_interval());
    EXPECT_EQ(reservation_repository.conflict_intervals().at(1), make_interval());
}

TEST(SearchAvailableResourcesHandlerTest, Handle_ShouldExcludeResourcesWithScheduleConflicts) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto available_resource_id = haven::domain::ResourceId{"resource-room-100"};
    const auto conflicting_resource_id = haven::domain::ResourceId{"resource-room-200"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    resource_repository.set_search_result(
        {make_resource(
             available_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom),
         make_resource(
             conflicting_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom)});
    reservation_repository.set_conflict_results({false, true});
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().resource_id(), available_resource_id);
}

TEST(SearchAvailableResourcesHandlerTest, Handle_ShouldReturnEmpty_WhenAllResourcesConflict) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto first_resource_id = haven::domain::ResourceId{"resource-room-100"};
    const auto second_resource_id = haven::domain::ResourceId{"resource-room-200"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    resource_repository.set_search_result(
        {make_resource(
             first_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom),
         make_resource(
             second_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom)});
    reservation_repository.set_conflict(true);
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    EXPECT_TRUE(result.empty());
}

TEST(SearchAvailableResourcesHandlerTest, Handle_ShouldReturnEmpty_WhenNoMatchingResourcesExist) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    auto resource_repository = TestResourceRepository{};
    auto reservation_repository = TestReservationRepository{};
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(reservation_repository.conflict_resource_ids().empty());
}

TEST(SearchAvailableResourcesHandlerTest,
     Handle_ShouldExcludeCrossTenantResources_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto leaked_organization_id = haven::domain::OrganizationId{"organization-beta"};
    const auto visible_resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_search_result(
        {make_resource(
             visible_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom),
         make_resource(haven::domain::ResourceId{"resource-room-200"},
                       leaked_organization_id,
                       haven::domain::ResourceType::MeetingRoom)});
    auto reservation_repository = TestReservationRepository{};
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().resource_id(), visible_resource_id);
    ASSERT_EQ(reservation_repository.conflict_resource_ids().size(), 1U);
    EXPECT_EQ(reservation_repository.conflict_resource_ids().front(), visible_resource_id);
}

TEST(SearchAvailableResourcesHandlerTest,
     Handle_ShouldExcludeWrongTypeResources_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto visible_resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_search_result(
        {make_resource(
             visible_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom),
         make_resource(haven::domain::ResourceId{"resource-desk-100"},
                       organization_id,
                       haven::domain::ResourceType::OfficeDesk)});
    auto reservation_repository = TestReservationRepository{};
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().resource_id(), visible_resource_id);
    ASSERT_EQ(reservation_repository.conflict_resource_ids().size(), 1U);
    EXPECT_EQ(reservation_repository.conflict_resource_ids().front(), visible_resource_id);
}

TEST(SearchAvailableResourcesHandlerTest,
     Handle_ShouldExcludeInactiveResources_WhenRepositoryLeaksData) {
    const auto organization_id = haven::domain::OrganizationId{"organization-alpha"};
    const auto visible_resource_id = haven::domain::ResourceId{"resource-room-100"};
    auto resource_repository = TestResourceRepository{};
    resource_repository.set_search_result(
        {make_resource(
             visible_resource_id, organization_id, haven::domain::ResourceType::MeetingRoom),
         make_resource(haven::domain::ResourceId{"resource-room-200"},
                       organization_id,
                       haven::domain::ResourceType::MeetingRoom,
                       haven::domain::ResourceStatus::Inactive)});
    auto reservation_repository = TestReservationRepository{};
    const auto handler =
        SearchAvailableResourcesHandler{resource_repository, reservation_repository};

    const auto result = handler.handle(SearchAvailableResourcesQuery{
        organization_id, haven::domain::ResourceType::MeetingRoom, make_interval()});

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result.front().resource_id(), visible_resource_id);
    ASSERT_EQ(reservation_repository.conflict_resource_ids().size(), 1U);
    EXPECT_EQ(reservation_repository.conflict_resource_ids().front(), visible_resource_id);
}

}  // namespace
}  // namespace haven::application::resources
