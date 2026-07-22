/**
 * @file main.cpp
 * @brief Defines the entry point and initial composition root for Haven.
 *
 * This file configures the HTTP listener, registers presentation routes, and
 * starts the Drogon event loop. Business logic and HTTP handler implementations
 * must remain outside the executable entry point.
 */

#include "haven/presentation/health/live_controller.hpp"

#include <drogon/HttpAppFramework.h>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {

constexpr std::string_view kDefaultHttpAddress{"0.0.0.0"};
constexpr std::string_view kDefaultHttpPort{"8080"};

/**
 * @brief Returns an environment-variable value or a default value.
 *
 * Empty environment variables are treated as absent so the process does not
 * accidentally start with an invalid blank configuration.
 *
 * @param variable Name of the environment variable.
 * @param default_value Value returned when the variable is absent or empty.
 * @return The configured or default value.
 */
[[nodiscard]] std::string_view environment_or_default(
    const char* variable,
    const std::string_view default_value) {
    const char* value = std::getenv(variable);

    if (value == nullptr || *value == '\0') {
        return default_value;
    }

    return value;
}

/**
 * @brief Parses and validates the configured HTTP listener port.
 *
 * @param value Decimal port representation.
 * @return A valid TCP port in the inclusive range `[1, 65535]`.
 *
 * @throws std::invalid_argument
 *     When the value is not a complete decimal integer or is outside the valid
 *     TCP port range.
 */
[[nodiscard]] std::uint16_t parse_http_port(const std::string_view value) {
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
        throw std::invalid_argument{
            "HAVEN_HTTP_PORT must be an integer between 1 and 65535"};
    }

    return static_cast<std::uint16_t>(parsed_port);
}

}  // namespace

int main() {
    try {
        const std::string http_address{
            environment_or_default(
                "HAVEN_HTTP_ADDRESS",
                kDefaultHttpAddress)};

        const std::uint16_t http_port =
            parse_http_port(
                environment_or_default(
                    "HAVEN_HTTP_PORT",
                    kDefaultHttpPort));

        haven::presentation::health::register_live_route();

        std::cout
            << "Starting Haven API on "
            << http_address
            << ':'
            << http_port
            << '\n';

        drogon::app()
            .addListener(http_address, http_port)
            .setThreadNum(1)
            .run();

        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr
            << "Haven failed to start: "
            << exception.what()
            << '\n';

        return EXIT_FAILURE;
    }
}