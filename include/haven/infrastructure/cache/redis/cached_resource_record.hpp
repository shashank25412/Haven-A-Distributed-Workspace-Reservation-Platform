/**
 * @file cached_resource_record.hpp
 * @brief Declares serialization helpers for Redis Resource cache records.
 */

#pragma once

#include "haven/domain/resource.hpp"

#include <string>

namespace haven::infrastructure::cache::redis {

[[nodiscard]] std::string serialize_cached_resource(const haven::domain::Resource& resource);
[[nodiscard]] haven::domain::Resource deserialize_cached_resource(const std::string& payload);

}  // namespace haven::infrastructure::cache::redis
