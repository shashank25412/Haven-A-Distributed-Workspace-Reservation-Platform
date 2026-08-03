/**
 * @file redis_resource_detail_cache.hpp
 * @brief Declares the Redis implementation of the Resource detail cache.
 */

#pragma once

#include "haven/application/observability/metrics/metrics_recorder.hpp"
#include "haven/application/resources/resource_detail_cache.hpp"
#include "haven/infrastructure/cache/redis/metrics/redis_resource_cache_metrics.hpp"
#include "haven/infrastructure/cache/redis/redis_configuration.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace haven::infrastructure::cache::redis {
class RedisConnection;

class RedisResourceDetailCache final : public haven::application::resources::ResourceDetailCache {
public:
    RedisResourceDetailCache(
        std::shared_ptr<RedisConnection> connection,
        RedisConfiguration configuration,
        application::observability::metrics::MetricsRecorder& metrics_recorder);
    [[nodiscard]] std::optional<haven::domain::Resource> find(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override;
    void store(const haven::domain::OrganizationId& organization_id,
               const haven::domain::Resource& resource) override;
    void erase(const haven::domain::OrganizationId& organization_id,
               const haven::domain::ResourceId& resource_id) override;

private:
    void record(metrics::Operation operation,
                metrics::Outcome outcome,
                std::chrono::steady_clock::time_point started_at) const noexcept;
    void erase_key(const std::string& key) const noexcept;

    std::shared_ptr<RedisConnection> connection_;
    RedisConfiguration configuration_;
    application::observability::metrics::MetricsRecorder& metrics_recorder_;
};
}  // namespace haven::infrastructure::cache::redis
