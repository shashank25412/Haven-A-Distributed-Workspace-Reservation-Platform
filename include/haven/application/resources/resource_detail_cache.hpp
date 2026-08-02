/**
 * @file resource_detail_cache.hpp
 * @brief Declares the application port for cached Resource details.
 */

#pragma once

#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"

#include <optional>
#include <stdexcept>

namespace haven::application::resources {

class ResourceDetailCacheError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ResourceDetailCache {
public:
    virtual ~ResourceDetailCache() = default;
    [[nodiscard]] virtual std::optional<haven::domain::Resource> find(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const = 0;
    virtual void store(const haven::domain::OrganizationId& organization_id,
                       const haven::domain::Resource& resource) = 0;
    virtual void erase(const haven::domain::OrganizationId& organization_id,
                       const haven::domain::ResourceId& resource_id) = 0;
};

}  // namespace haven::application::resources
