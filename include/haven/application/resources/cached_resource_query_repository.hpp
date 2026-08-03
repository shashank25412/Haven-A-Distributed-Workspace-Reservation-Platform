/**
 * @file cached_resource_query_repository.hpp
 * @brief Declares the cache-backed Resource query repository adapter.
 */

#pragma once

#include "haven/application/observability/metrics/metrics_recorder.hpp"
#include "haven/application/resources/resource_detail_cache.hpp"
#include "haven/application/resources/resource_query_repository.hpp"

namespace haven::application::resources {

class CachedResourceQueryRepository final : public ResourceQueryRepository {
public:
    CachedResourceQueryRepository(
        ResourceDetailCache& cache,
        ResourceQueryRepository& authoritative,
        observability::metrics::MetricsRecorder& metrics_recorder) noexcept;
    [[nodiscard]] ResourceQueryResult find_by_id(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override;

private:
    ResourceDetailCache& cache_;
    ResourceQueryRepository& authoritative_;
    observability::metrics::MetricsRecorder& metrics_recorder_;
};

}  // namespace haven::application::resources
