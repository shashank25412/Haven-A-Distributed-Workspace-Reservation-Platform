/** @file readiness_controller.cpp @brief Implements Haven's readiness endpoint. */
#include "haven/presentation/health/readiness_controller.hpp"

#include "haven/presentation/health/readiness_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

namespace haven::presentation::health {
void register_readiness_route(
    std::shared_ptr<const application::health::ReadinessService> readiness) {
    drogon::app().registerHandler(
        "/health/ready",
        [readiness = std::move(readiness)](
            const drogon::HttpRequestPtr&,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            const auto readiness_response = ReadinessResponse{readiness->check()};
            auto response = drogon::HttpResponse::newHttpJsonResponse(readiness_response.to_json());
            response->setStatusCode(readiness_response.status_code());
            callback(response);
        },
        {drogon::Get});
}
}  // namespace haven::presentation::health
