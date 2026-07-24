/**
 * @file configuration.cpp
 * @brief Implements Haven's environment-based process configuration.
 *
 * External string values are validated and converted into typed configuration
 * before they are exposed to the rest of the process.
 */

#include "haven/bootstrap/configuration.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
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

constexpr std::string_view kHttpAddressVariable{"HVN_HTTP_ADDRESS"};
constexpr std::string_view kHttpPortVariable{"HVN_HTTP_PORT"};
constexpr std::string_view kHttpThreadsVariable{"HVN_HTTP_THREADS"};
constexpr std::string_view kLogLevelVariable{"HVN_LOG_LEVEL"};

/**
 * @brief Reads an environment variable or returns its default value.
 *
 * An unset or empty environment variable is treated as absent.
 *
 * @param variable_name Name of the environment variable.
 * @param default_value Value returned when the variable is absent or empty.
 * @return The configured value or the documented default.
 */
[[nodiscard]] std::string environment_or_default(
    const std::string_view variable_name,
    const std::string_view default_value) {
    const std::string variable{variable_name};
    const char* configured_value = std::getenv(variable.c_str());

    if (configured_value == nullptr || *configured_value == '\0') {
        return std::string{default_value};
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
        throw ConfigurationError{
            "HVN_HTTP_ADDRESS must not be blank"};
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
[[nodiscard]] std::uint16_t parse_http_port(
    const std::string_view value) {
    unsigned int parsed_port = 0;

    const auto [parse_end, error] = std::from_chars(
        value.data(),
        value.data() + value.size(),
        parsed_port);

    const bool consumed_entire_value =
        parse_end == value.data() + value.size();

    if (error != std::errc{} ||
        !consumed_entire_value ||
        parsed_port == 0U ||
        parsed_port > 65535U) {
        throw ConfigurationError{
            "HVN_HTTP_PORT must be an integer between 1 and 65535"};
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
[[nodiscard]] std::uint32_t parse_http_threads(
    const std::string_view value) {
    std::uint32_t parsed_threads = 0;

    const auto [parse_end, error] = std::from_chars(
        value.data(),
        value.data() + value.size(),
        parsed_threads);

    const bool consumed_entire_value =
        parse_end == value.data() + value.size();

    if (error != std::errc{} ||
        !consumed_entire_value ||
        parsed_threads == 0U) {
        throw ConfigurationError{
            "HVN_HTTP_THREADS must be a positive integer"};
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
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
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
    std::string http_address = environment_or_default(
        kHttpAddressVariable,
        kDefaultHttpAddress);

    const std::string http_port = environment_or_default(
        kHttpPortVariable,
        kDefaultHttpPort);

    const std::string http_threads = environment_or_default(
        kHttpThreadsVariable,
        kDefaultHttpThreads);

    std::string log_level = environment_or_default(
        kLogLevelVariable,
        kDefaultLogLevel);

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
    };
}

}  // namespace haven::bootstrap