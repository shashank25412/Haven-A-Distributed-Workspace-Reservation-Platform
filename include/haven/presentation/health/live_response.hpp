/**
 * @file live_response.hpp
 * @brief Defines the framework-independent response model for process liveness.
 *
 * The response represents Haven's process-level liveness state. It intentionally
 * contains no Drogon, JSON, dependency-health, or infrastructure-specific types.
 */

#pragma once

#include <string>

namespace haven::presentation::health {

/**
 * @brief Represents the stable response returned by the liveness endpoint.
 *
 * Liveness indicates only that the Haven process is running and capable of
 * handling the request. It must not depend on Couchbase, Redis, Kafka, or any
 * other external service.
 */
struct LiveResponse final {
    std::string status;
    std::string service;
};

/**
 * @brief Creates Haven's stable process-liveness response.
 *
 * @return A response whose status is `alive` and whose service name is
 * `haven-api`.
 */
[[nodiscard]] LiveResponse make_live_response();

}  // namespace haven::presentation::health