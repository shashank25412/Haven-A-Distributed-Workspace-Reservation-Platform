/**
 * @file get_resource_handler.hpp
 * @brief Declares the GetResource application use-case handler.
 */

#pragma once

#include "haven/application/resources/get_resource_query.hpp"
#include "haven/application/resources/resource_repository.hpp"

namespace haven::application::resources {

/**
 * @brief Retrieves one resource while preserving tenant isolation.
 *
 * The handler does not know about HTTP, JSON, Drogon, Couchbase, Redis, or
 * any other infrastructure concern.
 */
class GetResourceHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param resource_repository Tenant-aware resource repository.
     */
    explicit GetResourceHandler(ResourceRepository& resource_repository) noexcept;

    /**
     * @brief Executes the tenant-scoped resource lookup.
     *
     * A missing resource and a resource owned by another organization produce
     * the same empty result.
     *
     * @param query Tenant and resource identifiers for the lookup.
     * @return The visible resource or an empty result.
     */
    [[nodiscard]] ResourceLookupResult handle(const GetResourceQuery& query) const;

private:
    ResourceRepository& resource_repository_;
};

}  // namespace haven::application::resources