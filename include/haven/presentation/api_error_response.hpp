/**
 * @file api_error_response.hpp
 * @brief Declares the generic HTTP API error response DTO.
 */

#pragma once

#include <json/value.h>
#include <string>

namespace haven::presentation {

class ApiErrorResponse final {
public:
    ApiErrorResponse(std::string code, std::string message, std::string trace_id);
    [[nodiscard]] Json::Value to_json() const;

private:
    Json::Value response_;
};

}  // namespace haven::presentation
