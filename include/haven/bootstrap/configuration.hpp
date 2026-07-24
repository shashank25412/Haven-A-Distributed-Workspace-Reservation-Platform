/**
 * @file configuration.hpp
 * @brief Declares Haven's validated process configuration contract.
 *
 * The bootstrap layer translates external configuration sources, such as
 * environment variables, into typed values used to compose the Haven process.
 *
 * Domain and application code must not read environment variables directly.
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace haven::bootstrap {

/**
 * @brief Defines the supported application log levels.
 *
 * This enum is owned by Haven and intentionally avoids exposing logging
 * framework types through the bootstrap configuration contract.
 */
enum class LogLevel {
    trace,
    debug,
    info,
    warn,
    error,
    critical,
};

/**
 * @brief Represents validated HTTP server configuration.
 *
 * Instances contain only validated values. Invalid external configuration must
 * be rejected before this type is returned to the composition root.
 */
struct HttpConfiguration final {
    /**
     * Address on which the HTTP server listens.
     *
     * Examples:
     * - `0.0.0.0`
     * - `127.0.0.1`
     * - `localhost`
     */
    std::string address;

    /**
     * TCP port on which the HTTP server listens.
     *
     * Valid range: `[1, 65535]`.
     */
    std::uint16_t port;

    /**
     * Number of Drogon event-loop worker threads.
     *
     * The value must be greater than zero.
     */
    std::uint32_t worker_threads;
};

/**
 * @brief Represents validated application logging configuration.
 */
struct LoggingConfiguration final {
    /**
     * Minimum severity emitted by the application logger.
     */
    LogLevel level;
};

/**
 * @brief Represents all validated configuration required to start Haven.
 *
 * Additional process-level sections will be introduced only when real
 * infrastructure dependencies are added.
 */
struct ApplicationConfiguration final {
    HttpConfiguration http;
    LoggingConfiguration logging;
};

/**
 * @brief Reports invalid or missing process configuration.
 *
 * This error belongs to the process bootstrap boundary. It must not propagate
 * into domain or application workflows.
 */
class ConfigurationError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Loads and validates Haven configuration from environment variables.
 *
 * Supported variables:
 *
 * - `HVN_HTTP_ADDRESS`
 * - `HVN_HTTP_PORT`
 * - `HVN_HTTP_THREADS`
 * - `HVN_LOG_LEVEL`
 *
 * Missing or empty variables use documented development defaults. Present but
 * invalid values cause configuration loading to fail.
 *
 * Supported log-level values are case-insensitive:
 *
 * - `trace`
 * - `debug`
 * - `info`
 * - `warn`
 * - `warning`
 * - `error`
 * - `critical`
 *
 * @return Fully validated process configuration.
 *
 * @throws ConfigurationError
 *     When a configured value cannot be parsed or violates its constraints.
 */
[[nodiscard]] ApplicationConfiguration load_configuration_from_environment();

}  // namespace haven::bootstrap