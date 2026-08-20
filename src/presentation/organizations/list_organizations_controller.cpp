/**
 * @file list_organizations_controller.cpp
 * @brief Implements the organization directory listing HTTP endpoint.
 */

#include "haven/presentation/organizations/list_organizations_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"
#include "haven/presentation/organizations/organization_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::organizations {
namespace {

[[nodiscard]] std::string trace_id(const drogon::HttpRequestPtr& request) {
    auto request_id = request->getHeader("X-Request-Id");
    return request_id.empty() ? "unavailable" : std::move(request_id);
}

[[nodiscard]] drogon::HttpResponsePtr error_response(const drogon::HttpRequestPtr& request,
                                                     const drogon::HttpStatusCode status,
                                                     std::string code,
                                                     std::string message) {
    const auto body =
        haven::presentation::ApiErrorResponse{std::move(code), std::move(message), trace_id(request)};
    auto response = drogon::HttpResponse::newHttpJsonResponse(body.to_json());
    response->setStatusCode(status);
    response->setContentTypeString("application/problem+json");
    return response;
}

void handle_list_organizations(
    const std::shared_ptr<haven::application::organizations::ListOrganizationsHandler>& handler,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    HVN_TRACE_SCOPE();

    try {
        const auto organizations = handler->handle();

        Json::Value body;
        body["items"] = Json::arrayValue;
        for (const auto& organization : organizations) {
            body["items"].append(OrganizationResponse{organization}.to_json());
        }

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k200OK);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    } catch (const haven::application::RepositoryError& repository_error) {
        using haven::application::RepositoryErrorCode;
        const auto unavailable = repository_error.code() == RepositoryErrorCode::Timeout ||
                                 repository_error.code() == RepositoryErrorCode::Authentication ||
                                 repository_error.code() == RepositoryErrorCode::Authorization;
        callback(error_response(request,
                                unavailable ? drogon::k503ServiceUnavailable
                                            : drogon::k500InternalServerError,
                                unavailable ? "SERVICE_UNAVAILABLE" : "INTERNAL_ERROR",
                                unavailable ? "Organization listing is temporarily unavailable."
                                            : "The organization listing could not be completed."));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Organization listing request failed unexpectedly");
        callback(error_response(request,
                                drogon::k500InternalServerError,
                                "INTERNAL_ERROR",
                                "The organization listing could not be completed."));
    }
}

}  // namespace

void register_list_organizations_route(
    std::shared_ptr<haven::application::organizations::ListOrganizationsHandler> handler) {
    if (!handler) {
        throw std::invalid_argument("Organization listing route configuration is invalid");
    }
    drogon::app().registerHandler(
        "/api/v1/organizations",
        [handler = std::move(handler)](const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            handle_list_organizations(handler, request, std::move(callback));
        },
        {drogon::Get});
}

}  // namespace haven::presentation::organizations
