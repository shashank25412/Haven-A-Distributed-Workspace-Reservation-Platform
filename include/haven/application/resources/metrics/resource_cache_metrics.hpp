/** @file resource_cache_metrics.hpp @brief Defines bounded cached-resource query metrics. */
#pragma once

#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"

#include <string_view>

namespace haven::application::resources::metrics {

enum class QuerySource { cache_hit, authoritative_after_miss, authoritative_after_cache_failure };

/** Counter of cached-query decisions, labelled only by bounded source. */
[[nodiscard]] const observability::metrics::MetricName& queries_metric_name();
/** Complete decorator latency, labelled by the same bounded source. */
[[nodiscard]] const observability::metrics::MetricName& query_duration_metric_name();
[[nodiscard]] std::string_view source_value(QuerySource source) noexcept;
[[nodiscard]] observability::metrics::MetricLabel source_label(QuerySource source);

}  // namespace haven::application::resources::metrics
