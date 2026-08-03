/** @file redis_resource_cache_metrics.cpp @brief Implements Redis Resource cache metrics. */
#include "haven/infrastructure/cache/redis/metrics/redis_resource_cache_metrics.hpp"

namespace haven::infrastructure::cache::redis::metrics {
const application::observability::metrics::MetricName& operations_metric_name() {
    static const application::observability::metrics::MetricName name{
        "haven_redis_resource_cache_operations_total"};
    return name;
}
const application::observability::metrics::MetricName& duration_metric_name() {
    static const application::observability::metrics::MetricName name{
        "haven_redis_resource_cache_operation_duration_seconds"};
    return name;
}
std::string_view value(const Operation operation) noexcept {
    switch (operation) {
        case Operation::find:
            return "find";
        case Operation::store:
            return "store";
        case Operation::erase:
            return "erase";
    }
    return "find";
}
std::string_view value(const Outcome outcome) noexcept {
    switch (outcome) {
        case Outcome::hit:
            return "hit";
        case Outcome::miss:
            return "miss";
        case Outcome::success:
            return "success";
        case Outcome::timeout:
            return "timeout";
        case Outcome::unavailable:
            return "unavailable";
        case Outcome::serialization_failed:
            return "serialization_failed";
        case Outcome::validation_failed:
            return "validation_failed";
        case Outcome::command_failed:
            return "command_failed";
        case Outcome::unexpected_failure:
            return "unexpected_failure";
    }
    return "unexpected_failure";
}
}  // namespace haven::infrastructure::cache::redis::metrics
