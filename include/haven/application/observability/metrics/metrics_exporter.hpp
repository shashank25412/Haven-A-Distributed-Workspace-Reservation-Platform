/**
 * @file metrics_exporter.hpp
 * @brief Declares the presentation-facing metrics export port.
 */

#pragma once

#include <string>

namespace haven::application::observability::metrics {

class MetricsExporter {
public:
    virtual ~MetricsExporter() = default;

    [[nodiscard]] virtual std::string collect() const = 0;
};

}  // namespace haven::application::observability::metrics
