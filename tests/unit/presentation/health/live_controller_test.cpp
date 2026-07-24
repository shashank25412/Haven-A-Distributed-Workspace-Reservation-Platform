/**
 * @file live_controller_test.cpp
 * @brief Tests Haven's liveness endpoint through the Drogon HTTP boundary.
 */

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon_test.h>

namespace {

constexpr char kContractTestServerAddress[] = "http://127.0.0.1:18080";
constexpr char kLiveEndpointPath[] = "/health/live";

}  // namespace

DROGON_TEST(GetLive_ShouldReturnUp_WhenApplicationIsRunning) {
    const auto client = drogon::HttpClient::newHttpClient(kContractTestServerAddress);

    const auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Get);
    request->setPath(kLiveEndpointPath);

    client->sendRequest(
        request,
        [TEST_CTX](const drogon::ReqResult result,
                   const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);

            CHECK(response->getStatusCode() == drogon::k200OK);
            CHECK(response->contentType() == drogon::CT_APPLICATION_JSON);

            const auto response_body = response->getJsonObject();

            REQUIRE(response_body != nullptr);
            CHECK(response_body->isObject());
            CHECK(response_body->isMember("status"));
            CHECK((*response_body)["status"].asString() == "UP");
            CHECK(response_body->size() == 1U);
        });
}