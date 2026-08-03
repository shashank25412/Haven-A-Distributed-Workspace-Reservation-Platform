/**
 * @file prometheus_metrics_recorder.cpp
 * @brief Implements the prometheus-cpp metrics adapter.
 */

#include "haven/infrastructure/observability/metrics/prometheus_metrics_recorder.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <mutex>
#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/labels.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>
#include <prometheus/text_serializer.h>
#include <stdexcept>
#include <string_view>

namespace haven::infrastructure::observability::metrics {
namespace {

enum class MetricType { counter, gauge, duration };

[[nodiscard]] bool valid_metric_name(const std::string_view name) {
    const auto valid_first = [](const unsigned char character) {
        return std::isalpha(character) != 0 || character == '_' || character == ':';
    };
    const auto valid_rest = [&](const unsigned char character) {
        return valid_first(character) || std::isdigit(character) != 0;
    };
    return !name.empty() && valid_first(static_cast<unsigned char>(name.front())) &&
           std::all_of(name.begin() + 1, name.end(), valid_rest);
}

[[nodiscard]] bool valid_label_name(const std::string_view name) {
    const auto valid_first = [](const unsigned char character) {
        return std::isalpha(character) != 0 || character == '_';
    };
    const auto valid_rest = [&](const unsigned char character) {
        return valid_first(character) || std::isdigit(character) != 0;
    };
    return !name.empty() && valid_first(static_cast<unsigned char>(name.front())) &&
           std::all_of(name.begin() + 1, name.end(), valid_rest);
}

[[nodiscard]] prometheus::Labels to_prometheus_labels(
    const haven::application::observability::metrics::MetricLabels& labels) {
    prometheus::Labels converted;
    for (const auto& label : labels) {
        if (!valid_label_name(label.name()))
            throw std::invalid_argument("Metric label name is not Prometheus-compatible");
        if (!converted.emplace(label.name(), label.value()).second)
            throw std::invalid_argument("Metric labels must not contain duplicate names");
    }
    return converted;
}

void validate_metric_name(const std::string_view name) {
    if (!valid_metric_name(name))
        throw std::invalid_argument("Metric name is not Prometheus-compatible");
}

}  // namespace

struct PrometheusMetricsRecorder::Impl final {
    void require_type(const std::string& name, const MetricType type) {
        const auto [iterator, inserted] = types.try_emplace(name, type);
        if (!inserted && iterator->second != type)
            throw std::logic_error("Metric name is already registered with an incompatible type");
    }

    mutable std::mutex mutex;
    prometheus::Registry registry;
    std::map<std::string, MetricType> types;
};

PrometheusMetricsRecorder::PrometheusMetricsRecorder() : impl_(std::make_unique<Impl>()) {}

PrometheusMetricsRecorder::~PrometheusMetricsRecorder() = default;

void PrometheusMetricsRecorder::increment_counter(
    const haven::application::observability::metrics::MetricName& name,
    const double amount,
    const haven::application::observability::metrics::MetricLabels& labels) {
    if (amount < 0.0 || std::isnan(amount))
        throw std::invalid_argument("Counter increment must be non-negative");
    validate_metric_name(name.value());
    auto converted = to_prometheus_labels(labels);
    const auto lock = std::scoped_lock{impl_->mutex};
    impl_->require_type(name.value(), MetricType::counter);
    auto& family = prometheus::BuildCounter().Name(name.value()).Help("").Register(impl_->registry);
    family.Add(converted).Increment(amount);
}

void PrometheusMetricsRecorder::set_gauge(
    const haven::application::observability::metrics::MetricName& name,
    const double value,
    const haven::application::observability::metrics::MetricLabels& labels) {
    validate_metric_name(name.value());
    auto converted = to_prometheus_labels(labels);
    const auto lock = std::scoped_lock{impl_->mutex};
    impl_->require_type(name.value(), MetricType::gauge);
    auto& family = prometheus::BuildGauge().Name(name.value()).Help("").Register(impl_->registry);
    family.Add(converted).Set(value);
}

void PrometheusMetricsRecorder::observe_duration(
    const haven::application::observability::metrics::MetricName& name,
    const std::chrono::microseconds duration,
    const haven::application::observability::metrics::MetricLabels& labels) {
    if (duration.count() < 0)
        throw std::invalid_argument("Metric duration must be non-negative");
    validate_metric_name(name.value());
    auto converted = to_prometheus_labels(labels);
    const auto seconds = std::chrono::duration<double>(duration).count();
    const auto lock = std::scoped_lock{impl_->mutex};
    impl_->require_type(name.value(), MetricType::duration);
    auto& family = prometheus::BuildSummary().Name(name.value()).Help("").Register(impl_->registry);
    family.Add(converted, prometheus::Summary::Quantiles{}).Observe(seconds);
}

std::string PrometheusMetricsRecorder::collect() const {
    const auto lock = std::scoped_lock{impl_->mutex};
    return prometheus::TextSerializer{}.Serialize(impl_->registry.Collect());
}

}  // namespace haven::infrastructure::observability::metrics
