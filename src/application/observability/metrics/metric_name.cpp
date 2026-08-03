/**
 * @file metric_name.cpp
 * @brief Implements the backend-independent metric name.
 */

#include "haven/application/observability/metrics/metric_name.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::observability::metrics {

MetricName::MetricName(std::string value) : value_(std::move(value)) {
    if (value_.empty()) {
        throw std::invalid_argument("Metric name must not be empty.");
    }
}

const std::string& MetricName::value() const noexcept {
    return value_;
}

}  // namespace haven::application::observability::metrics
