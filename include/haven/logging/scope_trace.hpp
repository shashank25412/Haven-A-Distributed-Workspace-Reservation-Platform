#pragma once

/**
 * @file scope_trace.hpp
 * @brief Declares the RAII utility used to trace function entry and exit.
 */

#include <chrono>
#include <string_view>

namespace haven::logging {

/**
 * @brief Traces the lifetime of a function scope.
 *
 * Construction emits a function-entry trace message. Destruction emits the
 * matching function-exit message together with the elapsed duration in
 * microseconds.
 *
 * The file path and function name are stored as non-owning string views.
 * Callers are expected to pass static compiler-provided values such as
 * `__FILE__` and `__func__`.
 *
 * Logging failures are suppressed by the logger.
 *
 * @thread_safety Separate instances may safely be used by different threads.
 */
class ScopeTrace final {
public:
    /**
     * @brief Creates a scope trace and records the scope start time.
     *
     * @param file_path Source file path associated with the traced function.
     * @param function_name Name of the traced function.
     */
    ScopeTrace(std::string_view file_path, std::string_view function_name) noexcept;

    /**
     * @brief Emits the function-exit trace and elapsed duration.
     */
    ~ScopeTrace() noexcept;

    ScopeTrace(const ScopeTrace&) = delete;
    ScopeTrace& operator=(const ScopeTrace&) = delete;
    ScopeTrace(ScopeTrace&&) = delete;
    ScopeTrace& operator=(ScopeTrace&&) = delete;

private:
    [[nodiscard]] static std::string_view extract_file_name(std::string_view file_path) noexcept;

    std::string_view file_name_;
    std::string_view function_name_;
    std::chrono::steady_clock::time_point start_time_;
};

}  // namespace haven::logging