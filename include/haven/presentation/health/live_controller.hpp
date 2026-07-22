/**
 * @file live_controller.hpp
 * @brief Declares registration of Haven's process-liveness HTTP route.
 *
 * This file belongs to the presentation layer because it exposes an
 * HTTP-facing capability. The public contract intentionally avoids Drogon
 * types so framework details remain inside the implementation file.
 */

#pragma once

namespace haven::presentation::health {

/**
 * @brief Registers the `GET /health/live` route with the HTTP application.
 *
 * The endpoint reports process liveness only. It must not query Couchbase,
 * Redis, Kafka, or any other external dependency.
 *
 * This function must be called before the Drogon event loop starts.
 */
void register_live_route();

}  // namespace haven::presentation::health