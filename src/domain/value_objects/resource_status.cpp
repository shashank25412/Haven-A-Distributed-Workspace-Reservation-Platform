/**
 * @file resource_status.cpp
 * @brief Implements conversion for supported Haven resource states.
 */

#include "haven/domain/value_objects/resource_status.hpp"

#include <stdexcept>
#include <string>

namespace haven::domain {

std::string_view to_string(const ResourceStatus resource_status) noexcept {
    switch (resource_status) {
        case ResourceStatus::Active:
            return "ACTIVE";
        case ResourceStatus::Inactive:
            return "INACTIVE";
    }

    return "UNKNOWN";
}

ResourceStatus resource_status_from_string(const std::string_view value) {
    if (value == "ACTIVE") {
        return ResourceStatus::Active;
    }
    if (value == "INACTIVE") {
        return ResourceStatus::Inactive;
    }

    throw std::invalid_argument(
        "Unsupported resource status: " + std::string{value});
}

}  // namespace haven::domain
