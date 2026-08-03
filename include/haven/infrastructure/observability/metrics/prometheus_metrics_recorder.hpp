/**
 * @file prometheus_metrics_recorder.hpp
 * @brief Declares a thread-safe Prometheus metrics registry and exporter.
 */

#pragma once

#include "haven/application/observability/metrics/metrics_exporter.hpp"
#include "haven/application/observability/metrics/metrics_recorder.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace haven::infrastructure::observability::metrics {

class PrometheusMetricsRecorder final
    : public haven::application::observability::metrics::MetricsRecorder,
      public haven::application::observability::metrics::MetricsExporter {
public:
    PrometheusMetricsRecorder();
    ~PrometheusMetricsRecorder() override;

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

    [[nodiscard]] std::string collect() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace haven::infrastructure::observability::metrics
