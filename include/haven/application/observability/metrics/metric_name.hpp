/**
 * @file metric_name.hpp
 * @brief Defines a backend-independent metric name.
 */

#pragma once

#include <string>

namespace haven::application::observability::metrics {

class MetricName final {
public:
    /** @throws std::invalid_argument If value is empty. */
    explicit MetricName(std::string value);

    [[nodiscard]] const std::string& value() const noexcept;

    bool operator==(const MetricName&) const = default;

private:
    std::string value_;
};

}  // namespace haven::application::observability::metrics
