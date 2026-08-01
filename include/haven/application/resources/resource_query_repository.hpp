#pragma once

#include "haven/domain/resource.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"

#include <optional>

namespace haven::application::resources {

using ResourceQueryResult = std::optional<haven::domain::Resource>;

class ResourceQueryRepository {
public:
    virtual ~ResourceQueryRepository() = default;
    [[nodiscard]] virtual ResourceQueryResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const = 0;
};

}  // namespace haven::application::resources
