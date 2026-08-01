#pragma once

#include "haven/application/resources/resource_detail_cache.hpp"
#include "haven/application/resources/resource_query_repository.hpp"

namespace haven::application::resources {

class CachedResourceQueryRepository final : public ResourceQueryRepository {
public:
    CachedResourceQueryRepository(ResourceDetailCache& cache,
                                  ResourceQueryRepository& authoritative) noexcept;
    [[nodiscard]] ResourceQueryResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override;

private:
    ResourceDetailCache& cache_;
    ResourceQueryRepository& authoritative_;
};

}  // namespace haven::application::resources
