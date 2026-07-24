/**
 * @file live_controller.cpp
 * @brief Implements Haven's liveness HTTP endpoint.
 *
 * The liveness endpoint confirms that the Haven process and HTTP server are
 * running. It does not inspect Couchbase, Redis, Kafka, or other dependencies.
 */

#include "haven/presentation/health/live_controller.hpp"

#include "haven/logging/logging.hpp"
#include "haven/presentation/health/live_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <functional>
#include <utility>

namespace haven::presentation::health {

namespace {

/**
 * @brief Handles a liveness request.
 *
 * @param request Incoming HTTP request.
 * @param callback Callback used to return the HTTP response.
 */
void handle_live_request(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    HVN_TRACE_SCOPE();
    HVN_DEBUG_LOG("Handling GET /health/live request");

    const LiveResponse live_response;

    auto response = drogon::HttpResponse::newHttpJsonResponse(live_response.to_json());
    response->setStatusCode(drogon::k200OK);

    callback(response);
}

}  // namespace

void register_live_route() {
    HVN_TRACE_SCOPE();
    HVN_DEBUG_LOG("Registering GET /health/live route");

    drogon::app().registerHandler("/health/live", &handle_live_request, {drogon::Get});
}

}  // namespace haven::presentation::health
