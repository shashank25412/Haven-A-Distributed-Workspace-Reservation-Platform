/**
 * @file scope_trace.cpp
 * @brief Implements RAII-based function scope tracing for Haven.
 */

#include "haven/logging/scope_trace.hpp"

#include "haven/logging/logger.hpp"

#include <chrono>

namespace haven::logging {

ScopeTrace::ScopeTrace(std::string_view file_path, std::string_view function_name) noexcept
    : file_name_(extract_file_name(file_path)),
      function_name_(function_name),
      start_time_(std::chrono::steady_clock::now()) {
    Logger::instance().log(LogLevel::Trace, "==> ", file_name_, "::", function_name_);
}

ScopeTrace::~ScopeTrace() noexcept {
    const auto end_time = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

    Logger::instance().log(LogLevel::Trace, "<== ", file_name_, "::", function_name_, " duration_us=", duration.count());
}

std::string_view ScopeTrace::extract_file_name(std::string_view file_path) noexcept {
    const std::size_t separator_position = file_path.find_last_of("/\\");

    if (separator_position == std::string_view::npos) {
        return file_path;
    }

    return file_path.substr(separator_position + 1);
}

}  // namespace haven::logging