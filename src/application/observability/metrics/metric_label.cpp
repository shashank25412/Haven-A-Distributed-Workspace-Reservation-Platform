/**
 * @file metric_label.cpp
 * @brief Implements the backend-independent metric label.
 */

#include "haven/application/observability/metrics/metric_label.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::observability::metrics {

MetricLabel::MetricLabel(std::string name, std::string value)
    : name_(std::move(name)), value_(std::move(value)) {
    if (name_.empty()) {
        throw std::invalid_argument("Metric label name must not be empty.");
    }
}

const std::string& MetricLabel::name() const noexcept {
    return name_;
}

const std::string& MetricLabel::value() const noexcept {
    return value_;
}

}  // namespace haven::application::observability::metrics
