/**
 * @file configuration.cpp
 * @brief Implements Haven's environment-based process configuration.
 *
 * External string values are validated and converted into typed configuration
 * before they are exposed to the rest of the process.
 */

#include "haven/bootstrap/configuration.hpp"

#include "haven/logging/logging.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace haven::bootstrap {
namespace {

constexpr std::string_view kDefaultHttpAddress{"0.0.0.0"};
constexpr std::string_view kDefaultHttpPort{"8080"};
constexpr std::string_view kDefaultHttpThreads{"1"};
constexpr std::string_view kDefaultLogLevel{"info"};
constexpr std::string_view kDefaultRedisEnabled{"false"};
constexpr std::string_view kDefaultRedisUri{"tcp://127.0.0.1:6379"};
constexpr std::string_view kDefaultRedisConnectTimeout{"100"};
constexpr std::string_view kDefaultRedisCommandTimeout{"100"};
constexpr std::string_view kDefaultRedisResourceTtl{"300"};

constexpr std::string_view kHttpAddressVariable{"HVN_HTTP_ADDRESS"};
constexpr std::string_view kHttpPortVariable{"HVN_HTTP_PORT"};
constexpr std::string_view kHttpThreadsVariable{"HVN_HTTP_THREADS"};
constexpr std::string_view kLogLevelVariable{"HVN_LOG_LEVEL"};
constexpr std::string_view kCouchbaseConnectionStringVariable{"HVN_COUCHBASE_CONNECTION_STRING"};
constexpr std::string_view kCouchbaseUsernameVariable{"HVN_COUCHBASE_USERNAME"};
constexpr std::string_view kCouchbasePasswordVariable{"HVN_COUCHBASE_PASSWORD"};
constexpr std::string_view kCouchbaseBucketVariable{"HVN_COUCHBASE_BUCKET"};
constexpr std::string_view kCouchbaseScopeVariable{"HVN_COUCHBASE_SCOPE"};
constexpr std::string_view kRedisEnabledVariable{"HVN_REDIS_ENABLED"};
constexpr std::string_view kRedisUriVariable{"HVN_REDIS_URI"};
constexpr std::string_view kRedisPasswordVariable{"HVN_REDIS_PASSWORD"};
constexpr std::string_view kRedisConnectTimeoutVariable{"HVN_REDIS_CONNECT_TIMEOUT_MS"};
constexpr std::string_view kRedisCommandTimeoutVariable{"HVN_REDIS_COMMAND_TIMEOUT_MS"};
constexpr std::string_view kRedisResourceTtlVariable{"HVN_REDIS_RESOURCE_TTL_SECONDS"};

[[nodiscard]] bool parse_bool(std::string value, const std::string_view variable) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (value == "true" || value == "1") {
        return true;
    }
    if (value == "false" || value == "0") {
        return false;
    }
    throw ConfigurationError{std::string{variable} + " must be true, false, 1, or 0"};
}

template <typename Duration>
[[nodiscard]] Duration parse_positive_duration(const std::string_view value,
                                               const std::string_view variable) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() || parsed == 0U) {
        throw ConfigurationError{std::string{variable} + " must be a positive integer"};
    }
    return Duration{parsed};
}

[[nodiscard]] std::string parse_redis_uri(std::string value) {
    if (!value.starts_with("tcp://") || value.size() <= std::string_view{"tcp://"}.size()) {
        throw ConfigurationError{"HVN_REDIS_URI must be a non-empty tcp:// URI"};
    }
    return value;
}

/**
 * @brief Reads an environment variable or returns its default value.
 *
 * An unset or empty environment variable is treated as absent.
 *
 * @param variable_name Name of the environment variable.
 * @param default_value Value returned when the variable is absent or empty.
 * @return The configured value or the documented default.
 */
[[nodiscard]] std::string environment_or_default(const std::string_view variable_name,
                                                 const std::string_view default_value) {
    const std::string variable{variable_name};
    const char* configured_value = std::getenv(variable.c_str());

    if (configured_value == nullptr || *configured_value == '\0') {
        return std::string{default_value};
    }

    return std::string{configured_value};
}

/**
 * @brief Reads a required, non-empty environment variable.
 *
 * @param variable_name Name of the environment variable.
 * @return The configured value, preserved exactly.
 *
 * @throws ConfigurationError
 *     When the variable is missing or empty.
 */
[[nodiscard]] std::string required_environment(const std::string_view variable_name) {
    HVN_TRACE_SCOPE();

    const std::string variable{variable_name};
    const char* configured_value = std::getenv(variable.c_str());

    if (configured_value == nullptr) {
        throw ConfigurationError{variable + " is required but is not set"};
    }

    if (*configured_value == '\0') {
        throw ConfigurationError{variable + " is required and must not be empty"};
    }

    return std::string{configured_value};
}

/**
 * @brief Validates the configured HTTP listener address.
 *
 * Detailed address parsing and resolution remain the responsibility of
 * Drogon and the operating system. Bootstrap validation rejects only blank
 * addresses.
 *
 * @param value Configured listener address.
 * @return The validated listener address.
 *
 * @throws ConfigurationError
 *     When the configured address contains only whitespace.
 */
[[nodiscard]] std::string parse_http_address(std::string value) {
    constexpr std::string_view kWhitespace{" \t\n\r\f\v"};

    if (value.find_first_not_of(kWhitespace) == std::string::npos) {
        throw ConfigurationError{"HVN_HTTP_ADDRESS must not be blank"};
    }

    return value;
}

/**
 * @brief Parses the configured HTTP listener port.
 *
 * The complete input must represent a decimal integer in the inclusive range
 * `[1, 65535]`.
 *
 * @param value Configured port value.
 * @return A validated TCP port.
 *
 * @throws ConfigurationError
 *     When the value is not a valid TCP port.
 */
[[nodiscard]] std::uint16_t parse_http_port(const std::string_view value) {
    unsigned int parsed_port = 0;

    const auto [parse_end, error] =
        std::from_chars(value.data(), value.data() + value.size(), parsed_port);

    const bool consumed_entire_value = parse_end == value.data() + value.size();

    if (error != std::errc{} || !consumed_entire_value || parsed_port == 0U ||
        parsed_port > 65535U) {
        throw ConfigurationError{"HVN_HTTP_PORT must be an integer between 1 and 65535"};
    }

    return static_cast<std::uint16_t>(parsed_port);
}

/**
 * @brief Parses the configured Drogon worker-thread count.
 *
 * Haven requires at least one event-loop worker thread.
 *
 * @param value Configured worker-thread count.
 * @return A validated positive thread count.
 *
 * @throws ConfigurationError
 *     When the value is not a positive decimal integer.
 */
[[nodiscard]] std::uint32_t parse_http_threads(const std::string_view value) {
    std::uint32_t parsed_threads = 0;

    const auto [parse_end, error] =
        std::from_chars(value.data(), value.data() + value.size(), parsed_threads);

    const bool consumed_entire_value = parse_end == value.data() + value.size();

    if (error != std::errc{} || !consumed_entire_value || parsed_threads == 0U) {
        throw ConfigurationError{"HVN_HTTP_THREADS must be a positive integer"};
    }

    return parsed_threads;
}

/**
 * @brief Converts text to lowercase using ASCII-compatible character rules.
 *
 * @param value Input text.
 * @return Lowercase copy of the input.
 */
[[nodiscard]] std::string to_lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    return value;
}

/**
 * @brief Parses Haven's configured minimum log severity.
 *
 * Supported values are case-insensitive. `warning` is accepted as an alias
 * for `warn`.
 *
 * @param value Configured log-level value.
 * @return The corresponding Haven-owned log-level enum.
 *
 * @throws ConfigurationError
 *     When the configured level is unsupported.
 */
[[nodiscard]] LogLevel parse_log_level(std::string value) {
    value = to_lowercase(std::move(value));

    if (value == "trace") {
        return LogLevel::trace;
    }

    if (value == "debug") {
        return LogLevel::debug;
    }

    if (value == "info") {
        return LogLevel::info;
    }

    if (value == "warn" || value == "warning") {
        return LogLevel::warn;
    }

    if (value == "error") {
        return LogLevel::error;
    }

    if (value == "critical") {
        return LogLevel::critical;
    }

    throw ConfigurationError{
        "HVN_LOG_LEVEL must be one of: "
        "trace, debug, info, warn, warning, error, critical"};
}

}  // namespace

ApplicationConfiguration load_configuration_from_environment() {
    HVN_TRACE_SCOPE();

    std::string http_address = environment_or_default(kHttpAddressVariable, kDefaultHttpAddress);

    const std::string http_port = environment_or_default(kHttpPortVariable, kDefaultHttpPort);

    const std::string http_threads =
        environment_or_default(kHttpThreadsVariable, kDefaultHttpThreads);

    std::string log_level = environment_or_default(kLogLevelVariable, kDefaultLogLevel);

    using infrastructure::cache::redis::RedisConfiguration;
    using infrastructure::persistence::couchbase::CouchbaseConfiguration;

    return ApplicationConfiguration{
        .http =
            HttpConfiguration{
                .address = parse_http_address(std::move(http_address)),
                .port = parse_http_port(http_port),
                .worker_threads = parse_http_threads(http_threads),
            },
        .logging =
            LoggingConfiguration{
                .level = parse_log_level(std::move(log_level)),
            },
        .couchbase =
            CouchbaseConfiguration{
                .connection_string = required_environment(kCouchbaseConnectionStringVariable),
                .username = required_environment(kCouchbaseUsernameVariable),
                .password = required_environment(kCouchbasePasswordVariable),
                .bucket_name = required_environment(kCouchbaseBucketVariable),
                .scope_name = required_environment(kCouchbaseScopeVariable),
            },
        .redis =
            RedisConfiguration{
                .enabled =
                    parse_bool(environment_or_default(kRedisEnabledVariable, kDefaultRedisEnabled),
                               kRedisEnabledVariable),
                .uri = parse_redis_uri(environment_or_default(kRedisUriVariable, kDefaultRedisUri)),
                .password = environment_or_default(kRedisPasswordVariable, ""),
                .connect_timeout = parse_positive_duration<std::chrono::milliseconds>(
                    environment_or_default(kRedisConnectTimeoutVariable,
                                           kDefaultRedisConnectTimeout),
                    kRedisConnectTimeoutVariable),
                .command_timeout = parse_positive_duration<std::chrono::milliseconds>(
                    environment_or_default(kRedisCommandTimeoutVariable,
                                           kDefaultRedisCommandTimeout),
                    kRedisCommandTimeoutVariable),
                .resource_detail_ttl = parse_positive_duration<std::chrono::seconds>(
                    environment_or_default(kRedisResourceTtlVariable, kDefaultRedisResourceTtl),
                    kRedisResourceTtlVariable),
            },
    };
}

}  // namespace haven::bootstrap
