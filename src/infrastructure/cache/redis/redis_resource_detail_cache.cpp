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

RedisResourceDetailCache::RedisResourceDetailCache(std::shared_ptr<RedisConnection> connection,
                                                   RedisConfiguration configuration)
    : connection_(std::move(connection)), configuration_(std::move(configuration)) {}

std::optional<haven::domain::Resource> RedisResourceDetailCache::find(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) const {
    const auto key = resource_detail_cache_key(organization_id, resource_id);
    try {
        const auto payload = connection_->client().get(key);
        if (!payload)
            return std::nullopt;
        try {
            auto resource = deserialize_cached_resource(*payload);
            if (resource.organization_id() != organization_id ||
                resource.resource_id() != resource_id) {
                throw std::invalid_argument{"Cache identity mismatch"};
            }
            return resource;
        } catch (const std::exception&) {
            HVN_WARN_LOG("Invalid Resource cache entry discarded");
            try {
                connection_->client().del(key);
            } catch (...) {}
            return std::nullopt;
        }
    } catch (const sw::redis::Error&) {
        throw ResourceDetailCacheError{"Redis Resource cache lookup failed"};
    }
}

void RedisResourceDetailCache::store(const haven::domain::OrganizationId& organization_id,
                                     const haven::domain::Resource& resource) {
    try {
        connection_->client().set(
            resource_detail_cache_key(organization_id, resource.resource_id()),
            serialize_cached_resource(resource),
            configuration_.resource_detail_ttl);
    } catch (const sw::redis::Error&) {
        throw ResourceDetailCacheError{"Redis Resource cache store failed"};
    }
}

void RedisResourceDetailCache::erase(const haven::domain::OrganizationId& organization_id,
                                     const haven::domain::ResourceId& resource_id) {
    try {
        connection_->client().del(resource_detail_cache_key(organization_id, resource_id));
    } catch (const sw::redis::Error&) {
        HVN_WARN_LOG("Resource detail cache erase failed");
    }
}
}  // namespace haven::infrastructure::cache::redis
