/**
 * @file recording_metrics_recorder.hpp
 * @brief Defines a deterministic recording metrics adapter for tests.
 */

#pragma once

#include "haven/application/observability/metrics/metrics_recorder.hpp"

#include <chrono>
#include <utility>
#include <vector>

namespace haven::test::application::observability::metrics {

using haven::application::observability::metrics::MetricLabels;
using haven::application::observability::metrics::MetricName;

struct CounterIncrement final {
    MetricName name;
    double amount;
    MetricLabels labels;

    bool operator==(const CounterIncrement&) const = default;
};

struct GaugeAssignment final {
    MetricName name;
    double value;
    MetricLabels labels;

    bool operator==(const GaugeAssignment&) const = default;
};

struct DurationObservation final {
    MetricName name;
    std::chrono::microseconds duration;
    MetricLabels labels;

    bool operator==(const DurationObservation&) const = default;
};

class RecordingMetricsRecorder final
    : public haven::application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(const MetricName& name,
                           double amount,
                           const MetricLabels& labels) override {
        counter_increments_.push_back(CounterIncrement{name, amount, labels});
    }

    void set_gauge(const MetricName& name, double value, const MetricLabels& labels) override {
        gauge_assignments_.push_back(GaugeAssignment{name, value, labels});
    }

    void observe_duration(const MetricName& name,
                          std::chrono::microseconds duration,
                          const MetricLabels& labels) override {
        duration_observations_.push_back(DurationObservation{name, duration, labels});
    }

    [[nodiscard]] const std::vector<CounterIncrement>& counter_increments() const noexcept {
        return counter_increments_;
    }

    [[nodiscard]] const std::vector<GaugeAssignment>& gauge_assignments() const noexcept {
        return gauge_assignments_;
    }

    [[nodiscard]] const std::vector<DurationObservation>& duration_observations() const noexcept {
        return duration_observations_;
    }

private:
    std::vector<CounterIncrement> counter_increments_;
    std::vector<GaugeAssignment> gauge_assignments_;
    std::vector<DurationObservation> duration_observations_;
};

}  // namespace haven::test::application::observability::metrics
