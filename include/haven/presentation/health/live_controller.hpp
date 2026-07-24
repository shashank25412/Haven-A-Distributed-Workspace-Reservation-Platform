/**
 * @file live_controller.hpp
 * @brief Declares registration for Haven's liveness endpoint.
 *
 * The liveness endpoint reports whether the Haven process and HTTP server
 * are running. It does not inspect external infrastructure dependencies.
 */

#pragma once

namespace haven::presentation::health {

/**
 * @brief Registers the Haven liveness HTTP route.
 *
 * Registers GET /health/live with the Drogon application. This function
 * must be called exactly once during application startup before the Drogon
 * event loop begins.
 */
void register_live_route();

}  // namespace haven::presentation::health