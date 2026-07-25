/**
 * @file resource_status.cpp
 * @brief Implements conversion for supported Haven resource states.
 */

#include "haven/domain/value_objects/resource_status.hpp"

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

}  // namespace haven::domain