/** @file resource_cache_metrics.cpp @brief Implements cached-resource query metrics. */
#include "haven/application/resources/metrics/resource_cache_metrics.hpp"

#include <string>

namespace haven::application::resources::metrics {
const observability::metrics::MetricName& queries_metric_name() {
    static const observability::metrics::MetricName name{"haven_resource_cache_queries_total"};
    return name;
}
const observability::metrics::MetricName& query_duration_metric_name() {
    static const observability::metrics::MetricName name{
        "haven_resource_cache_query_duration_seconds"};
    return name;
}
std::string_view source_value(const QuerySource source) noexcept {
    switch (source) {
        case QuerySource::cache_hit:
            return "cache_hit";
        case QuerySource::authoritative_after_miss:
            return "authoritative_after_miss";
        case QuerySource::authoritative_after_cache_failure:
            return "authoritative_after_cache_failure";
    }
    return "authoritative_after_cache_failure";
}
observability::metrics::MetricLabel source_label(const QuerySource source) {
    return {"source", std::string{source_value(source)}};
}
}  // namespace haven::application::resources::metrics
