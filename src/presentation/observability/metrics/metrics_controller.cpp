/**
 * @file metrics_controller.cpp
 * @brief Implements Haven's Prometheus scrape endpoint.
 */

#include "haven/presentation/observability/metrics/metrics_controller.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <functional>
#include <stdexcept>
#include <utility>

namespace haven::presentation::observability::metrics {

void register_metrics_route(
    std::shared_ptr<haven::application::observability::metrics::MetricsExporter> exporter) {
    if (!exporter)
        throw std::invalid_argument("Metrics route exporter must not be null");

    drogon::app().registerHandler(
        "/metrics",
        [exporter = std::move(exporter)](
            const drogon::HttpRequestPtr&,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k200OK);
            response->addHeader("Content-Type", "text/plain; version=0.0.4; charset=utf-8");
            response->setBody(exporter->collect());
            callback(response);
        },
        {drogon::Get});
}

}  // namespace haven::presentation::observability::metrics
