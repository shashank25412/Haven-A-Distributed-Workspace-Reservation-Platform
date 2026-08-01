/**
 * @file test_resource_repository.hpp
 * @brief Defines the Resource repository used by HTTP boundary tests.
 */

#pragma once

#include "haven/application/repository_error.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/version.hpp"

#include <optional>
#include <stdexcept>

namespace haven::presentation::resources::test {

class TestResourceRepository final
    : public haven::application::resources::ResourceRepository {
public:
    [[nodiscard]] haven::application::resources::ResourceLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override {
        if (resource_id.value() == "failure") {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence,
                "Sensitive test persistence details"};
        }

        if (resource_id.value() == "invalid-argument-failure") {
            throw std::invalid_argument{"Injected downstream invalid argument"};
        }

        if (organization_id.value() != "organization-1" || resource_id.value() != "resource-1") {
            return std::nullopt;
        }

        auto resource = haven::domain::Resource::rehydrate(
            haven::domain::OrganizationId{"organization-1"},
            haven::domain::ResourceId{"resource-1"},
            "Orion",
            "Meeting room near reception.",
            haven::domain::ResourceType::MeetingRoom,
            haven::domain::ResourceStatus::Active,
            true,
            haven::domain::Version{7});
        return haven::application::resources::LoadedResource{
            std::move(resource), haven::application::persistence::PersistenceToken{91}};
    }

    [[nodiscard]] haven::application::resources::ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId&,
        haven::domain::ResourceType) const override {
        return {};
    }
};

}  // namespace haven::presentation::resources::test
