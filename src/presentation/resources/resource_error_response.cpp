/**
 * @file resource_error_response.cpp
 * @brief Implements safe Resource error response serialization.
 */

#include "haven/presentation/resources/resource_error_response.hpp"

#include <utility>

namespace haven::presentation::resources {

ResourceErrorResponse::ResourceErrorResponse(
    std::string code,
    std::string message,
    std::string trace_id) {
    response_["code"] = std::move(code);
    response_["message"] = std::move(message);
    response_["details"] = Json::Value{Json::arrayValue};
    response_["traceId"] = std::move(trace_id);
}

Json::Value ResourceErrorResponse::to_json() const {
    return response_;
}

}  // namespace haven::presentation::resources
