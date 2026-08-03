/**
 * @file cached_resource_query_repository.cpp
 * @brief Implements cache-backed Resource queries.
 */

#include "haven/application/resources/cached_resource_query_repository.hpp"

#include "haven/application/resources/metrics/resource_cache_metrics.hpp"
#include "haven/logging/logging.hpp"

#include <chrono>

namespace haven::application::resources {

CachedResourceQueryRepository::CachedResourceQueryRepository(
    ResourceDetailCache& cache,
    ResourceQueryRepository& authoritative,
    observability::metrics::MetricsRecorder& metrics_recorder) noexcept
    : cache_(cache), authoritative_(authoritative), metrics_recorder_(metrics_recorder) {}

ResourceQueryResult CachedResourceQueryRepository::find_by_id(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) const {
    HVN_TRACE_SCOPE();
    const auto started_at = std::chrono::steady_clock::now();
    auto source = metrics::QuerySource::authoritative_after_miss;
    const auto record_terminal = [this, started_at](const metrics::QuerySource outcome) noexcept {
        const auto labels = [outcome] {
            return observability::metrics::MetricLabels{metrics::source_label(outcome)};
        };
        try {
            metrics_recorder_.increment_counter(metrics::queries_metric_name(), 1.0, labels());
        } catch (...) {}
        try {
            metrics_recorder_.observe_duration(
                metrics::query_duration_metric_name(),
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - started_at),
                labels());
        } catch (...) {}
    };
    try {
        if (auto cached = cache_.find(organization_id, resource_id); cached.has_value()) {
            HVN_DEBUG_LOG("Resource detail cache hit");
            record_terminal(metrics::QuerySource::cache_hit);
            return cached;
        }
        HVN_DEBUG_LOG("Resource detail cache miss");
    } catch (const ResourceDetailCacheError&) {
        source = metrics::QuerySource::authoritative_after_cache_failure;
        HVN_WARN_LOG("Resource detail cache lookup failed; using authoritative repository");
    }

    auto resource = ResourceQueryResult{};
    try {
        resource = authoritative_.find_by_id(organization_id, resource_id);
    } catch (...) {
        record_terminal(source);
        throw;
    }
    if (!resource.has_value()) {
        record_terminal(source);
        return std::nullopt;
    }

    try {
        cache_.store(organization_id, *resource);
    } catch (const ResourceDetailCacheError&) {
        HVN_WARN_LOG("Resource detail cache store failed; returning authoritative result");
    } catch (...) {
        record_terminal(source);
        throw;
    }
    record_terminal(source);
    return resource;
}

}  // namespace haven::application::resources
