/** @file readiness_controller.hpp @brief Declares Haven's readiness endpoint registration. */
#pragma once

#include "haven/application/health/readiness.hpp"

#include <memory>

namespace haven::presentation::health {
void register_readiness_route(
    std::shared_ptr<const application::health::ReadinessService> readiness);
}  // namespace haven::presentation::health
