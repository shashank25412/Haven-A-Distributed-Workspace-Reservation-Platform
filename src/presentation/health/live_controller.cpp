/**
 * @file live_controller.cpp
 * @brief Implements Haven's process-liveness HTTP endpoint.
 *
 * This file owns the Drogon-specific mapping between the framework-independent
 * liveness response model and the HTTP JSON response returned to clients.
 */

#include "haven/presentation/health/live_controller.hpp"

#include "haven/presentation/health/live_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <functional>
#include <utility>

namespace haven::presentation::health {

void register_live_route() {
    drogon::app().registerHandler(
        "/health/live",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            const LiveResponse live_response = make_live_response();

            Json::Value response_body;
            response_body["status"] = live_response.status;
            response_body["service"] = live_response.service;

            drogon::HttpResponsePtr response =
                drogon::HttpResponse::newHttpJsonResponse(response_body);

            response->setStatusCode(drogon::k200OK);
            response->addHeader("Cache-Control", "no-store");

            callback(std::move(response));
        },
        {drogon::Get});
}

}  // namespace haven::presentation::health