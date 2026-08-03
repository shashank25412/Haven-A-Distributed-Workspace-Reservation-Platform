/**
 * @file metrics_controller.hpp
 * @brief Declares registration for Haven's Prometheus scrape endpoint.
 */

#pragma once

#include "haven/application/observability/metrics/metrics_exporter.hpp"

#include <memory>

namespace haven::presentation::observability::metrics {

void register_metrics_route(
    std::shared_ptr<haven::application::observability::metrics::MetricsExporter> exporter);

}  // namespace haven::presentation::observability::metrics
