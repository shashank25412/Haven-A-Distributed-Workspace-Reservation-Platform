/**
 * @file metric_labels.hpp
 * @brief Defines an ordered collection of metric labels.
 */

#pragma once

#include "haven/application/observability/metrics/metric_label.hpp"

#include <vector>

namespace haven::application::observability::metrics {

using MetricLabels = std::vector<MetricLabel>;

}  // namespace haven::application::observability::metrics
