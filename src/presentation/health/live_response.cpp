/**
 * @file live_response.cpp
 * @brief Implements the response model for Haven's liveness endpoint.
 */

#include "haven/presentation/health/live_response.hpp"

namespace haven::presentation::health {

Json::Value LiveResponse::to_json() const {
    Json::Value response;
    response["status"] = "UP";
    return response;
}

}  // namespace haven::presentation::health