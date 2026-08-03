/**
 * @file metrics_recorder.hpp
 * @brief Declares the backend-independent metrics recording port.
 */

#pragma once

#include "haven/application/observability/metrics/metric_labels.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"

#include <chrono>

namespace haven::application::observability::metrics {

class MetricsRecorder {
public:
    virtual ~MetricsRecorder() = default;

    virtual void increment_counter(const MetricName& name,
                                   double amount,
                                   const MetricLabels& labels) = 0;
    virtual void set_gauge(const MetricName& name, double value, const MetricLabels& labels) = 0;
    virtual void observe_duration(const MetricName& name,
                                  std::chrono::microseconds duration,
                                  const MetricLabels& labels) = 0;
};

}  // namespace haven::application::observability::metrics
