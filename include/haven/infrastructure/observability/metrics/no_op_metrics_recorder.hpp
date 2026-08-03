/**
 * @file no_op_metrics_recorder.hpp
 * @brief Declares a metrics recorder that discards all observations.
 */

#pragma once

#include "haven/application/observability/metrics/metrics_recorder.hpp"

namespace haven::infrastructure::observability::metrics {

class NoOpMetricsRecorder final
    : public haven::application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(
        const haven::application::observability::metrics::MetricName& name,
        double amount,
        const haven::application::observability::metrics::MetricLabels& labels) override;
    void set_gauge(const haven::application::observability::metrics::MetricName& name,
                   double value,
                   const haven::application::observability::metrics::MetricLabels& labels) override;
    void observe_duration(
        const haven::application::observability::metrics::MetricName& name,
        std::chrono::microseconds duration,
        const haven::application::observability::metrics::MetricLabels& labels) override;
};

}  // namespace haven::infrastructure::observability::metrics
