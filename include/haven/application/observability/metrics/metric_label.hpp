/**
 * @file metric_label.hpp
 * @brief Defines a backend-independent metric label.
 */

#pragma once

#include <string>

namespace haven::application::observability::metrics {

class MetricLabel final {
public:
    /** @throws std::invalid_argument If name is empty. */
    MetricLabel(std::string name, std::string value);

    [[nodiscard]] const std::string& name() const noexcept;
    [[nodiscard]] const std::string& value() const noexcept;

    bool operator==(const MetricLabel&) const = default;

private:
    std::string name_;
    std::string value_;
};

}  // namespace haven::application::observability::metrics
