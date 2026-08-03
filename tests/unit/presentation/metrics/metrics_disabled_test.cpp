/**
 * @file metrics_disabled_test.cpp
 * @brief Tests that disabled metrics leave the scrape endpoint unregistered.
 */

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon_test.h>

DROGON_TEST(MetricsEndpointReturnsNotFoundWhenDisabled) {
    const auto client = drogon::HttpClient::newHttpClient("http://127.0.0.1:18084");
    const auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Get);
    request->setPath("/metrics");
    client->sendRequest(
        request,
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k404NotFound);
        });
}
