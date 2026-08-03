/**
 * @file no_op_metrics_recorder.cpp
 * @brief Implements a metrics recorder that discards all observations.
 */

#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"

namespace haven::infrastructure::observability::metrics {

void NoOpMetricsRecorder::increment_counter(
    const haven::application::observability::metrics::MetricName&,
    double,
    const haven::application::observability::metrics::MetricLabels&) {}

void NoOpMetricsRecorder::set_gauge(
    const haven::application::observability::metrics::MetricName&,
    double,
    const haven::application::observability::metrics::MetricLabels&) {}

void NoOpMetricsRecorder::observe_duration(
    const haven::application::observability::metrics::MetricName&,
    std::chrono::microseconds,
    const haven::application::observability::metrics::MetricLabels&) {}

}  // namespace haven::infrastructure::observability::metrics
