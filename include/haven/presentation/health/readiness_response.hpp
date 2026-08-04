/** @file readiness_response.hpp @brief Declares readiness HTTP response mapping. */
#pragma once

#include "haven/application/health/readiness.hpp"

#include <drogon/HttpTypes.h>

#include <json/value.h>

namespace haven::presentation::health {

class ReadinessResponse final {
public:
    explicit ReadinessResponse(application::health::ReadinessResult result) noexcept;

    [[nodiscard]] Json::Value to_json() const;
    [[nodiscard]] drogon::HttpStatusCode status_code() const noexcept;

private:
    application::health::ReadinessResult result_;
};

}  // namespace haven::presentation::health
