/**
 * @file live_response.hpp
 * @brief Declares the response model for Haven's liveness endpoint.
 *
 * This presentation-layer model converts the liveness status into the JSON
 * representation defined by the Haven API contract.
 */

#pragma once

#include <json/value.h>

namespace haven::presentation::health {

/**
 * @brief Represents a successful Haven liveness response.
 */
class LiveResponse final {
public:
    /**
     * @brief Converts the liveness response into JSON.
     *
     * @return JSON object containing `{"status":"UP"}`.
     */
    [[nodiscard]] Json::Value to_json() const;
};

}  // namespace haven::presentation::health