#pragma once

/**
 * @file logging.hpp
 * @brief Provides Haven's public logging macros and scope-tracing utility.
 *
 * Application code should normally include this header instead of including
 * the individual logging implementation headers directly.
 */

#include "haven/logging/logger.hpp"
#include "haven/logging/scope_trace.hpp"

/**
 * @brief Emits a trace-level log message.
 */
#define HVN_TRACE_LOG(...) ::haven::logging::Logger::instance().log(::haven::logging::LogLevel::Trace, __VA_ARGS__)

/**
 * @brief Emits a debug-level log message.
 */
#define HVN_DEBUG_LOG(...) ::haven::logging::Logger::instance().log(::haven::logging::LogLevel::Debug, __VA_ARGS__)

/**
 * @brief Emits an informational log message.
 */
#define HVN_INFO_LOG(...) ::haven::logging::Logger::instance().log(::haven::logging::LogLevel::Info, __VA_ARGS__)

/**
 * @brief Emits a warning-level log message.
 */
#define HVN_WARN_LOG(...) ::haven::logging::Logger::instance().log(::haven::logging::LogLevel::Warn, __VA_ARGS__)

/**
 * @brief Emits an error-level log message.
 */
#define HVN_ERROR_LOG(...) ::haven::logging::Logger::instance().log(::haven::logging::LogLevel::Error, __VA_ARGS__)

/**
 * @brief Emits a critical-level log message.
 */
#define HVN_CRITICAL_LOG(...) ::haven::logging::Logger::instance().log(::haven::logging::LogLevel::Critical, __VA_ARGS__)

#define HVN_DETAIL_CONCATENATE_IMPL(first, second) first##second
#define HVN_DETAIL_CONCATENATE(first, second) HVN_DETAIL_CONCATENATE_IMPL(first, second)

/**
 * @brief Traces entry into and exit from the current function scope.
 *
 * The generated local variable name includes the source line number so the
 * macro may be used in multiple scopes within the same function.
 */
#define HVN_TRACE_SCOPE() \
    ::haven::logging::ScopeTrace HVN_DETAIL_CONCATENATE(hvn_scope_trace_, __LINE__)(__FILE__, __func__)