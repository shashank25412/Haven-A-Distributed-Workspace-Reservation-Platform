/**
 * @file redis_resource_detail_cache.cpp
 * @brief Implements Redis persistence for cached Resource details.
 */

#include "haven/infrastructure/cache/redis/redis_resource_detail_cache.hpp"

#include "haven/infrastructure/cache/redis/cached_resource_record.hpp"
#include "haven/infrastructure/cache/redis/redis_connection.hpp"
#include "haven/infrastructure/cache/redis/resource_detail_cache_key.hpp"
#include "haven/logging/logging.hpp"

#include <sw/redis++/redis++.h>

namespace haven::infrastructure::cache::redis {
using haven::application::resources::ResourceDetailCacheError;

RedisResourceDetailCache::RedisResourceDetailCache(
    std::shared_ptr<RedisConnection> connection,
    RedisConfiguration configuration,
    application::observability::metrics::MetricsRecorder& metrics_recorder)
    : connection_(std::move(connection)),
      configuration_(std::move(configuration)),
      metrics_recorder_(metrics_recorder) {}

std::optional<haven::domain::Resource> RedisResourceDetailCache::find(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) const {
    const auto started_at = std::chrono::steady_clock::now();
    const auto key = resource_detail_cache_key(organization_id, resource_id);
    try {
        const auto payload = connection_->client().get(key);
        if (!payload) {
            record(metrics::Operation::find, metrics::Outcome::miss, started_at);
            return std::nullopt;
        }
        try {
            auto resource = deserialize_cached_resource(*payload);
            if (resource.organization_id() != organization_id ||
                resource.resource_id() != resource_id) {
                throw std::invalid_argument{"Cache identity mismatch"};
            }
            record(metrics::Operation::find, metrics::Outcome::hit, started_at);
            return resource;
        } catch (const std::exception&) {
            HVN_WARN_LOG("Invalid Resource cache entry discarded");
            record(metrics::Operation::find, metrics::Outcome::validation_failed, started_at);
            erase_key(key);
            return std::nullopt;
        }
    } catch (const sw::redis::TimeoutError&) {
        record(metrics::Operation::find, metrics::Outcome::timeout, started_at);
        throw ResourceDetailCacheError{"Redis Resource cache lookup failed"};
    } catch (const sw::redis::IoError&) {
        record(metrics::Operation::find, metrics::Outcome::unavailable, started_at);
        throw ResourceDetailCacheError{"Redis Resource cache lookup failed"};
    } catch (const sw::redis::Error&) {
        record(metrics::Operation::find, metrics::Outcome::command_failed, started_at);
        throw ResourceDetailCacheError{"Redis Resource cache lookup failed"};
    } catch (...) {
        record(metrics::Operation::find, metrics::Outcome::unexpected_failure, started_at);
        throw;
    }
}

void RedisResourceDetailCache::store(const haven::domain::OrganizationId& organization_id,
                                     const haven::domain::Resource& resource) {
    const auto started_at = std::chrono::steady_clock::now();
    std::string payload;
    try {
        payload = serialize_cached_resource(resource);
    } catch (...) {
        record(metrics::Operation::store, metrics::Outcome::serialization_failed, started_at);
        throw;
    }
    try {
        connection_->client().set(
            resource_detail_cache_key(organization_id, resource.resource_id()),
            payload,
            configuration_.resource_detail_ttl);
        record(metrics::Operation::store, metrics::Outcome::success, started_at);
    } catch (const sw::redis::TimeoutError&) {
        record(metrics::Operation::store, metrics::Outcome::timeout, started_at);
        throw ResourceDetailCacheError{"Redis Resource cache store failed"};
    } catch (const sw::redis::IoError&) {
        record(metrics::Operation::store, metrics::Outcome::unavailable, started_at);
        throw ResourceDetailCacheError{"Redis Resource cache store failed"};
    } catch (const sw::redis::Error&) {
        record(metrics::Operation::store, metrics::Outcome::command_failed, started_at);
        throw ResourceDetailCacheError{"Redis Resource cache store failed"};
    } catch (...) {
        record(metrics::Operation::store, metrics::Outcome::unexpected_failure, started_at);
        throw;
    }
}

void RedisResourceDetailCache::erase(const haven::domain::OrganizationId& organization_id,
                                     const haven::domain::ResourceId& resource_id) {
    erase_key(resource_detail_cache_key(organization_id, resource_id));
}

void RedisResourceDetailCache::erase_key(const std::string& key) const noexcept {
    const auto started_at = std::chrono::steady_clock::now();
    try {
        connection_->client().del(key);
        record(metrics::Operation::erase, metrics::Outcome::success, started_at);
    } catch (const sw::redis::TimeoutError&) {
        record(metrics::Operation::erase, metrics::Outcome::timeout, started_at);
        HVN_WARN_LOG("Resource detail cache erase failed");
    } catch (const sw::redis::IoError&) {
        record(metrics::Operation::erase, metrics::Outcome::unavailable, started_at);
        HVN_WARN_LOG("Resource detail cache erase failed");
    } catch (const sw::redis::Error&) {
        record(metrics::Operation::erase, metrics::Outcome::command_failed, started_at);
        HVN_WARN_LOG("Resource detail cache erase failed");
    } catch (...) {
        record(metrics::Operation::erase, metrics::Outcome::unexpected_failure, started_at);
        HVN_WARN_LOG("Resource detail cache erase failed");
    }
}

void RedisResourceDetailCache::record(
    const metrics::Operation operation,
    const metrics::Outcome outcome,
    const std::chrono::steady_clock::time_point started_at) const noexcept {
    const auto labels = [operation, outcome] {
        return application::observability::metrics::MetricLabels{
            {"operation", std::string{metrics::value(operation)}},
            {"outcome", std::string{metrics::value(outcome)}}};
    };
    try {
        metrics_recorder_.increment_counter(metrics::operations_metric_name(), 1.0, labels());
    } catch (...) {}
    try {
        metrics_recorder_.observe_duration(metrics::duration_metric_name(),
                                           std::chrono::duration_cast<std::chrono::microseconds>(
                                               std::chrono::steady_clock::now() - started_at),
                                           labels());
    } catch (...) {}
}
}  // namespace haven::infrastructure::cache::redis
