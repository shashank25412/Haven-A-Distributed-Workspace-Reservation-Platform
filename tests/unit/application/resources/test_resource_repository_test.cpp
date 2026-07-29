/**
 * @file test_resource_repository_test.cpp
 * @brief Verifies the Resource repository test double's loaded-result contract.
 */

#include "application/util/test_resource_repository.hpp"

#include "haven/application/persistence/persistence_token.hpp"
#include "haven/domain/resource.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace haven::application::resources {
namespace {

[[nodiscard]] haven::domain::Resource make_resource() {
    return haven::domain::Resource::rehydrate(haven::domain::OrganizationId{"organization-1"},
                                              haven::domain::ResourceId{"resource-1"},
                                              "Boardroom",
                                              "",
                                              haven::domain::ResourceType::MeetingRoom,
                                              haven::domain::ResourceStatus::Active,
                                              false,
                                              haven::domain::Version{7});
}

TEST(TestResourceRepositoryTest, FindByIdReturnsAggregateAndDeterministicToken) {
    auto repository = haven::tests::util::application::TestResourceRepository{};
    repository.set_lookup_result(make_resource());

    const auto first = repository.find_by_id(haven::domain::OrganizationId{"organization-1"},
                                             haven::domain::ResourceId{"resource-1"});
    const auto second = repository.find_by_id(haven::domain::OrganizationId{"organization-1"},
                                              haven::domain::ResourceId{"resource-1"});

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->aggregate().version().value(), 7U);
    EXPECT_EQ(first->persistence_token(), second->persistence_token());
}

TEST(TestResourceRepositoryTest, MissingPointLookupReturnsEmpty) {
    const auto repository = haven::tests::util::application::TestResourceRepository{};

    EXPECT_FALSE(repository.find_by_id(haven::domain::OrganizationId{"organization-1"},
                                       haven::domain::ResourceId{"missing"}));
}

TEST(TestResourceRepositoryTest, SearchResultsRemainAggregateOnly) {
    static_assert(std::is_same_v<ResourceSearchResult, std::vector<haven::domain::Resource>>);
    auto repository = haven::tests::util::application::TestResourceRepository{};
    repository.set_search_result({make_resource()});

    const auto resources = repository.find_active_by_type(
        haven::domain::OrganizationId{"organization-1"}, haven::domain::ResourceType::MeetingRoom);

    ASSERT_EQ(resources.size(), 1U);
    EXPECT_EQ(resources.front().version().value(), 7U);
}

}  // namespace
}  // namespace haven::application::resources
