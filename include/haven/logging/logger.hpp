#pragma once

/**
 * @file logger.hpp
 * @brief Declares Haven's thread-safe application logger.
 */

#include "haven/logging/log_level.hpp"

#include <atomic>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace haven::logging {

/**
 * @brief Provides process-wide, thread-safe logging for Haven.
 *
 * `Logger` is implemented as a singleton because Haven currently uses one
 * logging configuration and one output stream for the entire process.
 *
 * Messages are filtered according to the configured minimum log level.
 * Writes to the output stream are serialized so that messages from different
 * threads are not interleaved.
 *
 * Logging failures are suppressed and must never interrupt application flow.
 *
 * @thread_safety All public operations are safe to call concurrently.
 */
class Logger final {
public:
    /**
     * @brief Returns the process-wide logger instance.
     *
     * @return Reference to the logger instance.
     */
    [[nodiscard]] static Logger& instance() noexcept;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Sets the minimum severity required for a message to be emitted.
     *
     * Setting the level to `LogLevel::Off` disables all logging.
     *
     * @param level Minimum enabled log level.
     */
    void set_level(LogLevel level) noexcept;

    /**
     * @brief Returns the currently configured minimum log level.
     *
     * @return Currently configured log level.
     */
    [[nodiscard]] LogLevel level() const noexcept;

    /**
     * @brief Determines whether a message should be emitted.
     *
     * @param message_level Severity of the candidate message.
     * @return `true` when the message is enabled; otherwise, `false`.
     */
    [[nodiscard]] bool should_log(LogLevel message_level) const noexcept;

    /**
     * @brief Changes the stream used for future log output.
     *
     * The logger does not own the supplied stream. The caller must ensure that
     * the stream remains valid until another stream is configured.
     *
     * This function is primarily intended for tests and controlled application
     * bootstrap code.
     *
     * @param output Output stream that receives future log messages.
     */
    void set_output(std::ostream& output) noexcept;

    /**
     * @brief Builds and emits a log message from the supplied arguments.
     *
     * Arguments are appended from left to right using `operator<<`.
     * The complete message is emitted atomically with respect to other logging
     * threads.
     *
     * Any exception raised while formatting or writing the message is
     * suppressed.
     *
     * @tparam Arguments Types that support insertion into `std::ostream`.
     * @param message_level Severity assigned to the message.
     * @param arguments Values used to construct the message.
     */
    template <typename... Arguments>
    void log(LogLevel message_level, Arguments&&... arguments) noexcept {
        if (!should_log(message_level)) {
            return;
        }

        try {
            std::ostringstream message;
            (message << ... << std::forward<Arguments>(arguments));
            write(message_level, message.str());
        } catch (...) {
            // Logging failures must never affect application flow.
        }
    }

private:
    Logger() noexcept;

    void write(LogLevel message_level, std::string_view message);

    std::atomic<LogLevel> level_;
    std::ostream* output_;
    mutable std::mutex output_mutex_;
};

}  // namespace haven::logging