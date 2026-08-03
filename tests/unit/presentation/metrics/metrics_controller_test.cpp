/**
 * @file metrics_controller_test.cpp
 * @brief Tests the enabled Prometheus scrape HTTP endpoint.
 */

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon_test.h>

#include <functional>
#include <string>
#include <utility>

namespace {

const auto kClient = drogon::HttpClient::newHttpClient("http://127.0.0.1:18083");

void request_metrics(
    std::function<void(drogon::ReqResult, const drogon::HttpResponsePtr&)>&& done) {
    const auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Get);
    request->setPath("/metrics");
    kClient->sendRequest(request, std::move(done));
}

}  // namespace

DROGON_TEST(MetricsEndpointReturnsPrometheusPayloadWithoutIdentityHeaders) {
    request_metrics([TEST_CTX](const drogon::ReqResult result,
                               const drogon::HttpResponsePtr& response) {
        REQUIRE(result == drogon::ReqResult::Ok);
        REQUIRE(response != nullptr);
        CHECK(response->getStatusCode() == drogon::k200OK);
        CHECK(response->getHeader("Content-Type") == "text/plain; version=0.0.4; charset=utf-8");
        const std::string body{response->body()};
        CHECK(body.find("haven_test_requests_total{result=\"ok\"} 3") != std::string::npos);
        CHECK(body.find("haven_test_active_items 2") != std::string::npos);
        CHECK(body.find("haven_test_duration_seconds_count 1") != std::string::npos);
        CHECK(body.find("haven_test_duration_seconds_sum 0.5") != std::string::npos);
        CHECK(body.find("0x") == std::string::npos);
    });
}

DROGON_TEST(MetricsEndpointScrapingDoesNotClearMetrics) {
    request_metrics([TEST_CTX](const drogon::ReqResult first_result,
                               const drogon::HttpResponsePtr& first_response) {
        REQUIRE(first_result == drogon::ReqResult::Ok);
        REQUIRE(first_response != nullptr);
        const std::string first_body{first_response->body()};
        request_metrics([TEST_CTX, first_body](const drogon::ReqResult second_result,
                                               const drogon::HttpResponsePtr& second_response) {
            REQUIRE(second_result == drogon::ReqResult::Ok);
            REQUIRE(second_response != nullptr);
            CHECK(std::string{second_response->body()} == first_body);
        });
    });
}
