#pragma once

/**
 * @file log_level.hpp
 * @brief Defines the severity levels supported by Haven's logging module.
 */

#include <string_view>

namespace haven::logging {

/**
 * @brief Represents the severity of a log message.
 *
 * Log levels are ordered from the most verbose level, `Trace`, to `Off`.
 * A logger configured at a particular level emits messages at that level or
 * any level with greater severity.
 */
enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

/**
 * @brief Converts a log level to its uppercase textual representation.
 *
 * @param level Log level to convert.
 * @return A non-owning string view containing the textual representation.
 */
[[nodiscard]] constexpr std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Critical:
            return "CRITICAL";
        case LogLevel::Off:
            return "OFF";
    }

    return "UNKNOWN";
}

}  // namespace haven::logging