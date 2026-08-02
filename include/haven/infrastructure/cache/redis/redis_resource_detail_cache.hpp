/**
 * @file redis_resource_detail_cache.hpp
 * @brief Declares the Redis implementation of the Resource detail cache.
 */

#pragma once

#include "haven/application/resources/resource_detail_cache.hpp"
#include "haven/infrastructure/cache/redis/redis_configuration.hpp"

#include <memory>

namespace haven::infrastructure::cache::redis {
class RedisConnection;

class RedisResourceDetailCache final : public haven::application::resources::ResourceDetailCache {
public:
    RedisResourceDetailCache(std::shared_ptr<RedisConnection> connection,
                             RedisConfiguration configuration);
    [[nodiscard]] std::optional<haven::domain::Resource> find(
        const haven::domain::OrganizationId& organization_id,
        const haven::domain::ResourceId& resource_id) const override;
    void store(const haven::domain::OrganizationId& organization_id,
               const haven::domain::Resource& resource) override;
    void erase(const haven::domain::OrganizationId& organization_id,
               const haven::domain::ResourceId& resource_id) override;

private:
    std::shared_ptr<RedisConnection> connection_;
    RedisConfiguration configuration_;
};
}  // namespace haven::infrastructure::cache::redis
