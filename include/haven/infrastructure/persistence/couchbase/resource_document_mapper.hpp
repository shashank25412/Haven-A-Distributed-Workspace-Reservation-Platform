/**
 * @file resource_document_mapper.hpp
 * @brief Declares mapping between resources and Couchbase documents.
 */

#pragma once

#include "haven/domain/resource.hpp"
#include "haven/infrastructure/persistence/couchbase/resource_document.hpp"

namespace haven::infrastructure::persistence::couchbase {

/**
 * @brief Maps a domain resource to its Couchbase persistence representation.
 *
 * @param resource Resource aggregate to map.
 * @return Validated persistence document.
 */
[[nodiscard]] ResourceDocument to_resource_document(
    const domain::Resource& resource);

/**
 * @brief Rehydrates a domain resource from a Couchbase persistence document.
 *
 * @param document Persisted resource document.
 * @return Rehydrated resource aggregate.
 */
[[nodiscard]] domain::Resource to_domain_resource(
    const ResourceDocument& document);

}  // namespace haven::infrastructure::persistence::couchbase
