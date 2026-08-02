/**
 * @file resource_detail_cache_key.hpp
 * @brief Declares tenant-safe Redis keys for cached Resource details.
 */

#pragma once

#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"

#include <string>

namespace haven::infrastructure::cache::redis {

[[nodiscard]] std::string resource_detail_cache_key(
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id);

}  // namespace haven::infrastructure::cache::redis
