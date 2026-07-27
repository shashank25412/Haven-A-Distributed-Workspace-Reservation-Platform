/**
 * @file test_resource_repository.hpp
 * @brief Defines a configurable resource repository for application tests.
 */

#pragma once

#include "haven/application/resources/resource_repository.hpp"

#include <optional>
#include <utility>

namespace haven::tests::util::application {

/**
 * @brief Provides configurable resource repository behavior for application tests.
 *
 * Query results are empty by default. Each operation records whether it was
 * called and retains copies of the supplied arguments.
 */
class TestResourceRepository final : public haven::application::resources::ResourceRepository {
public:
    void set_lookup_result(haven::application::resources::ResourceLookupResult result) {
        lookup_result_ = std::move(result);
    }

    void set_search_result(haven::application::resources::ResourceSearchResult result) {
        search_result_ = std::move(result);
    }

    [[nodiscard]] haven::application::resources::ResourceLookupResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override {
        find_by_id_called_ = true;
        lookup_organization_id_ = organization_id;
        lookup_resource_id_ = resource_id;
        return lookup_result_;
    }

    [[nodiscard]] haven::application::resources::ResourceSearchResult find_active_by_type(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceType resource_type) const override {
        find_active_by_type_called_ = true;
        search_organization_id_ = organization_id;
        search_resource_type_ = resource_type;
        return search_result_;
    }

    [[nodiscard]] bool find_by_id_called() const noexcept {
        return find_by_id_called_;
    }

    [[nodiscard]] bool find_active_by_type_called() const noexcept {
        return find_active_by_type_called_;
    }

    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& lookup_organization_id()
        const noexcept {
        return lookup_organization_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::ResourceId>& lookup_resource_id()
        const noexcept {
        return lookup_resource_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& search_organization_id()
        const noexcept {
        return search_organization_id_;
    }

    [[nodiscard]] const std::optional<haven::domain::ResourceType>& search_resource_type()
        const noexcept {
        return search_resource_type_;
    }

private:
    haven::application::resources::ResourceLookupResult lookup_result_;
    haven::application::resources::ResourceSearchResult search_result_;
    mutable bool find_by_id_called_{false};
    mutable bool find_active_by_type_called_{false};
    mutable std::optional<haven::domain::OrganizationId> lookup_organization_id_;
    mutable std::optional<haven::domain::ResourceId> lookup_resource_id_;
    mutable std::optional<haven::domain::OrganizationId> search_organization_id_;
    mutable std::optional<haven::domain::ResourceType> search_resource_type_;
};

}  // namespace haven::tests::util::application
