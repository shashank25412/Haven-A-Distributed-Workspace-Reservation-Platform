/**
 * @file list_organizations_handler_test.cpp
 * @brief Tests the ListOrganizations application handler.
 */

#include "haven/application/organizations/list_organizations_handler.hpp"

#include "haven/application/organizations/organization_repository.hpp"
#include "haven/domain/organization.hpp"
#include "haven/domain/value_objects/organization_id.hpp"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace haven::application::organizations {
namespace {

class InMemoryOrganizationRepository final : public OrganizationRepository {
public:
    void add(haven::domain::Organization organization) {
        organizations_.push_back(std::move(organization));
    }

    [[nodiscard]] std::vector<haven::domain::Organization> find_all() const override {
        return organizations_;
    }

private:
    std::vector<haven::domain::Organization> organizations_;
};

TEST(ListOrganizationsHandlerTest, Handle_ShouldReturnEmptyList_WhenNoneAreRegistered) {
    auto repository = InMemoryOrganizationRepository{};
    const auto handler = ListOrganizationsHandler{repository};

    EXPECT_TRUE(handler.handle().empty());
}

TEST(ListOrganizationsHandlerTest, Handle_ShouldReturnEveryOrganization_WhenSomeAreRegistered) {
    auto repository = InMemoryOrganizationRepository{};
    repository.add(
        haven::domain::Organization{haven::domain::OrganizationId{"organization-1"}, "Organization One"});
    repository.add(
        haven::domain::Organization{haven::domain::OrganizationId{"organization-2"}, "Organization Two"});
    const auto handler = ListOrganizationsHandler{repository};

    const auto organizations = handler.handle();

    ASSERT_EQ(organizations.size(), 2U);
    EXPECT_EQ(organizations[0].name(), "Organization One");
    EXPECT_EQ(organizations[1].name(), "Organization Two");
}

}  // namespace
}  // namespace haven::application::organizations
