/**
 * @file logger.cpp
 * @brief Implements Haven's thread-safe application logger.
 */

#include "haven/logging/logger.hpp"

#include <iostream>

namespace haven::logging {

Logger& Logger::instance() noexcept {
    static Logger logger;
    return logger;
}

Logger::Logger() noexcept : level_(LogLevel::Info), output_(&std::clog) {}

void Logger::set_level(LogLevel level) noexcept {
    level_.store(level, std::memory_order_relaxed);
}

LogLevel Logger::level() const noexcept {
    return level_.load(std::memory_order_relaxed);
}

bool Logger::should_log(LogLevel message_level) const noexcept {
    const LogLevel configured_level = level();

    if (configured_level == LogLevel::Off) {
        return false;
    }

    return static_cast<int>(message_level) >= static_cast<int>(configured_level);
}

void Logger::set_output(std::ostream& output) noexcept {
    try {
        const std::scoped_lock lock(output_mutex_);
        output_ = &output;
    } catch (...) {
        // Logging configuration failures must not affect application flow.
    }
}

void Logger::write(LogLevel message_level, std::string_view message) {
    const std::scoped_lock lock(output_mutex_);

    *output_ << '[' << to_string(message_level) << "] " << message << '\n';
    output_->flush();
}

}  // namespace haven::logging