/**
 * @file live_response.cpp
 * @brief Implements the framework-independent liveness response.
 */

#include "haven/presentation/health/live_response.hpp"

namespace haven::presentation::health {

LiveResponse make_live_response() {
    return LiveResponse{
        .status = "alive",
        .service = "haven-api",
    };
}

}  // namespace haven::presentation::health