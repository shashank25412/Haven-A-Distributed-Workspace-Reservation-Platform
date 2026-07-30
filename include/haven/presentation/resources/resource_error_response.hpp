/**
 * @file resource_error_response.hpp
 * @brief Declares safe error serialization for Resource detail requests.
 */

#pragma once

#include <json/value.h>

#include <string>

namespace haven::presentation::resources {

/**
 * @brief Represents a safe Resource HTTP error response.
 */
class ResourceErrorResponse final {
public:
    ResourceErrorResponse(std::string code, std::string message, std::string trace_id);

    /** @brief Returns JSON matching the OpenAPI ErrorResponse schema. */
    [[nodiscard]] Json::Value to_json() const;

private:
    Json::Value response_;
};

}  // namespace haven::presentation::resources
