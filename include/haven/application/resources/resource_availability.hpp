/**
 * @file resource_availability.hpp
 * @brief Defines a resource paired with its remaining unit availability.
 */

#pragma once

#include "haven/domain/resource.hpp"

#include <cstdint>
#include <vector>

namespace haven::application::resources {

/**
 * @brief Pairs a resource with how many of its units remain bookable for a
 * requested interval.
 *
 * Single-unit resources (a specific meeting room, a specific desk) always
 * report an available_units of 1 when present in a result. Bulk/pooled
 * resources (a parking lot, a block of shared desks) may report more.
 */
struct ResourceAvailability {
    haven::domain::Resource resource;
    std::uint32_t available_units;
};

using ResourceAvailabilityResult = std::vector<ResourceAvailability>;

}  // namespace haven::application::resources
