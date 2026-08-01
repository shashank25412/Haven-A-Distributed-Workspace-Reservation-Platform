#pragma once

#include "haven/application/resources/resource_query_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"

namespace haven::application::resources {

class AuthoritativeResourceQueryRepository final : public ResourceQueryRepository {
public:
    explicit AuthoritativeResourceQueryRepository(ResourceRepository& repository) noexcept;
    [[nodiscard]] ResourceQueryResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override;

private:
    ResourceRepository& repository_;
};

}  // namespace haven::application::resources
