#include "haven/application/resources/cached_resource_query_repository.hpp"

#include "haven/logging/logging.hpp"

namespace haven::application::resources {

CachedResourceQueryRepository::CachedResourceQueryRepository(
    ResourceDetailCache& cache, ResourceQueryRepository& authoritative) noexcept
    : cache_(cache), authoritative_(authoritative) {}

ResourceQueryResult CachedResourceQueryRepository::find_by_id(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) const {
    HVN_TRACE_SCOPE();
    try {
        if (auto cached = cache_.find(organization_id, resource_id); cached.has_value()) {
            HVN_DEBUG_LOG("Resource detail cache hit");
            return cached;
        }
        HVN_DEBUG_LOG("Resource detail cache miss");
    } catch (const ResourceDetailCacheError&) {
        HVN_WARN_LOG("Resource detail cache lookup failed; using authoritative repository");
    }

    auto resource = authoritative_.find_by_id(organization_id, resource_id);
    if (!resource.has_value()) {
        return std::nullopt;
    }

    try {
        cache_.store(organization_id, *resource);
    } catch (const ResourceDetailCacheError&) {
        HVN_WARN_LOG("Resource detail cache store failed; returning authoritative result");
    }
    return resource;
}

}  // namespace haven::application::resources
