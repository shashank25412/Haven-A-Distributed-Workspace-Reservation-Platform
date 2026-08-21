/**
 * @file create_resource_controller.cpp
 * @brief Implements the admin resource-creation HTTP route.
 */

#include "haven/presentation/admin/create_resource_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"
#include "haven/presentation/resources/resource_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace haven::presentation::admin {
namespace {

constexpr char kRoute[]{"/api/v1/admin/resources"};

bool has_admin_role(const std::string& role) {
    constexpr std::array<std::string_view, 4> admin_roles{
        "APPROVER", "RESOURCE_ADMIN", "ORG_ADMIN", "SYSTEM_ADMIN"};
    return std::any_of(admin_roles.begin(), admin_roles.end(), [&role](const auto candidate) {
        return role == candidate;
    });
}

std::optional<std::string> bearer_token(const drogon::HttpRequestPtr& request) {
    constexpr std::string_view prefix{"Bearer "};
    const auto authorization = request->getHeader("Authorization");
    if (!authorization.starts_with(prefix) || authorization.size() <= prefix.size()) {
        return std::nullopt;
    }
    return authorization.substr(prefix.size());
}

std::string trace_id(const drogon::HttpRequestPtr& request) {
    auto value = request->getHeader("X-Request-Id");
    return value.empty() ? "unavailable" : std::move(value);
}

drogon::HttpResponsePtr error(const drogon::HttpRequestPtr& request,
                              const drogon::HttpStatusCode status,
                              std::string code,
                              std::string message) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        haven::presentation::ApiErrorResponse{std::move(code), std::move(message), trace_id(request)}
            .to_json());
    response->setStatusCode(status);
    response->setContentTypeString("application/problem+json");
    return response;
}

void handle(
    const std::shared_ptr<haven::application::resources::CreateResourceHandler>& handler,
    const std::shared_ptr<haven::application::auth::AuthenticationService>& authentication,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    HVN_TRACE_SCOPE();

    const auto token = bearer_token(request);
    if (!token) {
        callback(error(request,
                       drogon::k401Unauthorized,
                       "INVALID_SESSION",
                       "A bearer session is required."));
        return;
    }

    haven::application::auth::AuthenticatedAccount account;
    try {
        account = authentication->authenticate(*token);
    } catch (const haven::application::auth::AuthenticationError& auth_error) {
        const auto unavailable =
            auth_error.code() == haven::application::auth::AuthenticationErrorCode::persistence;
        callback(error(request,
                       unavailable ? drogon::k503ServiceUnavailable : drogon::k401Unauthorized,
                       unavailable ? "AUTHENTICATION_UNAVAILABLE" : "INVALID_SESSION",
                       unavailable ? "Authentication is temporarily unavailable."
                                   : "The authentication session is invalid or expired."));
        return;
    }

    if (!has_admin_role(account.role)) {
        callback(error(request,
                       drogon::k403Forbidden,
                       "FORBIDDEN",
                       "The caller is not authorized to manage resources."));
        return;
    }

    const auto* json = request->getJsonObject().get();
    if (json == nullptr) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST", "Request body is required."));
        return;
    }

    const auto get_string = [&](const char* field) -> std::optional<std::string> {
        if (!json->isMember(field) || !(*json)[field].isString()) return std::nullopt;
        auto value = (*json)[field].asString();
        if (value.empty()) return std::nullopt;
        return value;
    };

    const auto name = get_string("name");
    if (!name) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST", "Field 'name' is required."));
        return;
    }
    const auto description = get_string("description").value_or("");
    const auto resource_type_str = get_string("resourceType");
    if (!resource_type_str) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST", "Field 'resourceType' is required."));
        return;
    }

    haven::domain::ResourceType resource_type;
    try {
        resource_type = haven::domain::resource_type_from_string(*resource_type_str);
    } catch (const std::invalid_argument&) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST",
                       "Unknown resourceType: " + *resource_type_str));
        return;
    }

    const bool requires_approval =
        json->isMember("requiresApproval") && (*json)["requiresApproval"].isBool()
            ? (*json)["requiresApproval"].asBool()
            : false;

    const std::uint32_t total_units =
        json->isMember("totalUnits") && (*json)["totalUnits"].isUInt()
            ? (*json)["totalUnits"].asUInt()
            : 1U;

    const auto address = get_string("address").value_or("");

    try {
        const auto resource_id = haven::domain::ResourceId{drogon::utils::getUuid(true)};
        const haven::application::resources::CreateResourceCommand command{
            haven::domain::OrganizationId{account.organization_id},
            resource_id,
            *name,
            description,
            resource_type,
            requires_approval,
            total_units,
            address};

        const auto resource = handler->handle(command);

        auto response = drogon::HttpResponse::newHttpJsonResponse(
            haven::presentation::resources::ResourceResponse{resource}.to_json());
        response->setStatusCode(drogon::k201Created);
        return callback(response);
    } catch (const std::invalid_argument& validation_error) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST", validation_error.what()));
    } catch (const haven::application::RepositoryError& repo_error) {
        if (repo_error.code() == haven::application::RepositoryErrorCode::AlreadyExists) {
            callback(error(request, drogon::k409Conflict, "RESOURCE_CONFLICT",
                           "A resource with this identifier already exists."));
        } else {
            HVN_ERROR_LOG("Resource creation failed: ", repo_error.what());
            callback(error(request, drogon::k503ServiceUnavailable, "SERVICE_UNAVAILABLE",
                           "Resource creation is temporarily unavailable."));
        }
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Resource creation request failed unexpectedly");
        callback(error(request, drogon::k500InternalServerError, "INTERNAL_ERROR",
                       "An unexpected error occurred."));
    }
}

}  // namespace

void register_create_resource_route(
    std::shared_ptr<haven::application::resources::CreateResourceHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    drogon::app().registerHandler(
        kRoute,
        [handler = std::move(handler), authentication = std::move(authentication)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) mutable {
            handle(handler, authentication, request, std::move(callback));
        },
        {drogon::Post});
}

}  // namespace haven::presentation::admin
