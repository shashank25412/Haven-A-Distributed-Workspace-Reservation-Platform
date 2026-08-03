/** @file redis_resource_cache_metrics.hpp @brief Defines bounded Redis Resource cache metrics. */
#pragma once

#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"

#include <string_view>

namespace haven::infrastructure::cache::redis::metrics {
enum class Operation { find, store, erase };
enum class Outcome {
    hit,
    miss,
    success,
    timeout,
    unavailable,
    serialization_failed,
    validation_failed,
    command_failed,
    unexpected_failure
};
/** Counter of raw Redis adapter calls, with bounded operation and outcome labels. */
[[nodiscard]] const application::observability::metrics::MetricName& operations_metric_name();
/** Complete raw Redis adapter latency with matching bounded labels. */
[[nodiscard]] const application::observability::metrics::MetricName& duration_metric_name();
[[nodiscard]] std::string_view value(Operation operation) noexcept;
[[nodiscard]] std::string_view value(Outcome outcome) noexcept;
}  // namespace haven::infrastructure::cache::redis::metrics
