/**
 * @file get_resource_controller.cpp
 * @brief Implements the tenant-scoped Resource detail HTTP endpoint.
 */

#include "haven/presentation/resources/get_resource_controller.hpp"

#include "haven/application/resources/get_resource_query.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/resources/resource_error_response.hpp"
#include "haven/presentation/resources/resource_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::resources {
namespace {

constexpr char kResourceDetailRoute[] =
    "/api/v1/organizations/{organizationId}/resources/{resourceId}";
constexpr std::size_t kMaximumPathIdentifierLength = 255U;

[[nodiscard]] bool is_invalid_identifier(const std::string& value) {
    return value.empty() || value.size() > kMaximumPathIdentifierLength ||
           std::any_of(value.begin(), value.end(), [](const unsigned char character) {
               return std::iscntrl(character) != 0 || std::isspace(character) != 0;
           });
}

[[nodiscard]] std::string trace_id(const drogon::HttpRequestPtr& request) {
    auto request_id = request->getHeader("X-Request-Id");
    return request_id.empty() ? "unavailable" : std::move(request_id);
}

[[nodiscard]] drogon::HttpResponsePtr error_response(
    const drogon::HttpRequestPtr& request,
    const drogon::HttpStatusCode status,
    std::string code,
    std::string message) {
    const ResourceErrorResponse error{
        std::move(code), std::move(message), trace_id(request)};
    auto response = drogon::HttpResponse::newHttpJsonResponse(error.to_json());
    response->setStatusCode(status);
    response->setContentTypeString("application/problem+json");
    return response;
}

void handle_get_resource_request(
    const std::shared_ptr<haven::application::resources::GetResourceHandler>& handler,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string organization_id,
    std::string resource_id) {
    HVN_TRACE_SCOPE();

    if (is_invalid_identifier(organization_id) || is_invalid_identifier(resource_id)) {
        callback(error_response(request,
                                drogon::k400BadRequest,
                                "INVALID_REQUEST",
                                "Organization and resource identifiers must be valid."));
        return;
    }

    std::optional<haven::application::resources::GetResourceQuery> query;
    try {
        query.emplace(
            haven::domain::OrganizationId{std::move(organization_id)},
            haven::domain::ResourceId{std::move(resource_id)});
    } catch (const std::invalid_argument&) {
        callback(error_response(request,
                                drogon::k400BadRequest,
                                "INVALID_REQUEST",
                                "The request contains an invalid identifier."));
        return;
    }

    try {
        const auto resource = handler->handle(*query);

        if (!resource.has_value()) {
            callback(error_response(request,
                                    drogon::k404NotFound,
                                    "RESOURCE_NOT_FOUND",
                                    "The requested resource was not found."));
            return;
        }

        const ResourceResponse resource_response{*resource};
        auto response = drogon::HttpResponse::newHttpJsonResponse(resource_response.to_json());
        response->setStatusCode(drogon::k200OK);
        callback(response);
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Resource detail request failed unexpectedly");
        callback(error_response(request,
                                drogon::k500InternalServerError,
                                "INTERNAL_ERROR",
                                "The request could not be completed."));
    } catch (...) {
        HVN_ERROR_LOG("Resource detail request failed with an unknown error");
        callback(error_response(request,
                                drogon::k500InternalServerError,
                                "INTERNAL_ERROR",
                                "The request could not be completed."));
    }
}

}  // namespace

void register_get_resource_route(
    std::shared_ptr<haven::application::resources::GetResourceHandler> handler) {
    HVN_TRACE_SCOPE();

    if (!handler) {
        throw std::invalid_argument("Get Resource route handler must not be null");
    }

    drogon::app().registerHandler(
        kResourceDetailRoute,
        [handler = std::move(handler)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            std::string organization_id,
            std::string resource_id) {
            handle_get_resource_request(
                handler,
                request,
                std::move(callback),
                std::move(organization_id),
                std::move(resource_id));
        },
        {drogon::Get});
}

}  // namespace haven::presentation::resources
