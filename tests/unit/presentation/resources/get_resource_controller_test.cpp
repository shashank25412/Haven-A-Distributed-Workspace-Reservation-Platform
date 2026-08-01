/**
 * @file get_resource_controller_test.cpp
 * @brief Tests Resource detail behavior through the Drogon HTTP boundary.
 */

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon_test.h>

#include <string>

namespace {

constexpr char kTestServerAddress[] = "http://127.0.0.1:18081";

void send_get(
    const std::string& path,
    std::function<void(const drogon::ReqResult, const drogon::HttpResponsePtr&)>&& callback) {
    const auto client = drogon::HttpClient::newHttpClient(kTestServerAddress);
    const auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Get);
    request->setPath(path);
    client->sendRequest(request, std::move(callback));
}

}  // namespace

DROGON_TEST(GetResource_ShouldReturnCompleteResource_WhenTenantScopedResourceExists) {
    send_get(
        "/api/v1/organizations/organization-1/resources/resource-1",
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k200OK);

            const auto body = response->getJsonObject();
            REQUIRE(body != nullptr);
            CHECK((*body)["organizationId"].asString() == "organization-1");
            CHECK((*body)["resourceId"].asString() == "resource-1");
            CHECK((*body)["name"].asString() == "Orion");
            CHECK((*body)["description"].asString() == "Meeting room near reception.");
            CHECK((*body)["resourceType"].asString() == "MEETING_ROOM");
            CHECK((*body)["status"].asString() == "ACTIVE");
            CHECK((*body)["requiresApproval"].asBool());
            CHECK((*body)["version"].asUInt64() == 7U);
            CHECK(body->size() == 8U);
            CHECK(!body->isMember("persistenceToken"));
            CHECK(!body->isMember("cas"));
        });
}

DROGON_TEST(GetResource_ShouldReturnNotFound_WhenResourceIsMissingForCorrectOrganization) {
    send_get(
        "/api/v1/organizations/organization-1/resources/missing-resource",
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k404NotFound);
            CHECK(response->getHeader("Content-Type") == "application/problem+json");
            const auto body = response->getJsonObject();
            REQUIRE(body != nullptr);
            CHECK((*body)["code"].asString() == "RESOURCE_NOT_FOUND");
        });
}

DROGON_TEST(GetResource_ShouldReturnNotFound_ForWrongTenantResource) {
    send_get(
        "/api/v1/organizations/organization-2/resources/resource-1",
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k404NotFound);
            CHECK(response->getHeader("Content-Type") == "application/problem+json");
            const auto body = response->getJsonObject();
            REQUIRE(body != nullptr);
            CHECK((*body)["code"].asString() == "RESOURCE_NOT_FOUND");
        });
}

DROGON_TEST(GetResource_ShouldReturnInternalError_WhenDownstreamThrowsInvalidArgument) {
    send_get(
        "/api/v1/organizations/organization-1/resources/invalid-argument-failure",
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k500InternalServerError);
            CHECK(response->getHeader("Content-Type") == "application/problem+json");
            const auto body = response->getJsonObject();
            REQUIRE(body != nullptr);
            CHECK((*body)["code"].asString() == "INTERNAL_ERROR");
            CHECK(body->toStyledString().find("Injected downstream invalid argument") ==
                  std::string::npos);
        });
}

DROGON_TEST(GetResource_ShouldReturnBadRequest_WhenOrganizationIdentifierIsInvalid) {
    const std::string invalid_identifier(256U, 'a');
    send_get(
        "/api/v1/organizations/" + invalid_identifier + "/resources/resource-1",
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k400BadRequest);
        });
}

DROGON_TEST(GetResource_ShouldReturnBadRequest_WhenResourceIdentifierIsInvalid) {
    const std::string invalid_identifier(256U, 'a');
    send_get(
        "/api/v1/organizations/organization-1/resources/" + invalid_identifier,
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k400BadRequest);
        });
}

DROGON_TEST(GetResource_ShouldReturnSafeInternalError_WhenRepositoryFails) {
    send_get(
        "/api/v1/organizations/organization-1/resources/failure",
        [TEST_CTX](const drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
            REQUIRE(result == drogon::ReqResult::Ok);
            REQUIRE(response != nullptr);
            CHECK(response->getStatusCode() == drogon::k500InternalServerError);
            const auto body = response->getJsonObject();
            REQUIRE(body != nullptr);
            CHECK((*body)["code"].asString() == "INTERNAL_ERROR");
            CHECK((*body)["message"].asString() == "The request could not be completed.");
            CHECK(body->toStyledString().find("Sensitive test persistence details") ==
                  std::string::npos);
        });
}
