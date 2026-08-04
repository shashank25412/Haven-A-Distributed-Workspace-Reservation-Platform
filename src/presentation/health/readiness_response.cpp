/** @file readiness_response.cpp @brief Implements readiness HTTP response mapping. */
#include "haven/presentation/health/readiness_response.hpp"

namespace haven::presentation::health {

ReadinessResponse::ReadinessResponse(application::health::ReadinessResult result) noexcept
    : result_(result) {}

Json::Value ReadinessResponse::to_json() const {
    Json::Value body;
    body["status"] = application::health::to_string(result_.status);
    body["components"]["couchbase"] = application::health::to_string(result_.couchbase);
    body["components"]["redis"] = application::health::to_string(result_.redis);
    body["components"]["kafka"] = application::health::to_string(result_.kafka);
    body["components"]["outboxPublisher"] =
        application::health::to_string(result_.outbox_publisher);
    return body;
}

drogon::HttpStatusCode ReadinessResponse::status_code() const noexcept {
    return result_.status == application::health::ReadinessStatus::ready
               ? drogon::k200OK
               : drogon::k503ServiceUnavailable;
}

}  // namespace haven::presentation::health
