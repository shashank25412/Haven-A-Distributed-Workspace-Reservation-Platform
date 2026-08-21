/**
 * @file delete_resource_controller.cpp
 * @brief Implements DELETE /api/v1/admin/resources/{resourceId} — permanently remove a resource.
 */

#include "haven/presentation/admin/delete_resource_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace haven::presentation::admin {
namespace {

constexpr char kRoute[]{"/api/v1/admin/resources/{resourceId}"};
constexpr std::size_t kMaxIdLength{255};

bool has_admin_role(const std::string& role) {
    constexpr std::array<std::string_view, 4> admin_roles{
        "APPROVER", "RESOURCE_ADMIN", "ORG_ADMIN", "SYSTEM_ADMIN"};
    return std::any_of(admin_roles.begin(), admin_roles.end(), [&role](const auto candidate) {
        return role == candidate;
    });
}

bool is_invalid_identifier(const std::string& value) {
    return value.empty() || value.size() > kMaxIdLength ||
           std::any_of(value.begin(), value.end(), [](const unsigned char c) {
               return std::iscntrl(c) != 0 || std::isspace(c) != 0;
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
    const std::shared_ptr<haven::application::resources::ResourceRepository>& resource_repository,
    const std::shared_ptr<haven::application::auth::AuthenticationService>& authentication,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string resource_id) {
    HVN_TRACE_SCOPE();

    if (is_invalid_identifier(resource_id)) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST",
                       "The resource identifier is invalid."));
        return;
    }

    const auto token = bearer_token(request);
    if (!token) {
        callback(error(request, drogon::k401Unauthorized, "INVALID_SESSION",
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
        callback(error(request, drogon::k403Forbidden, "FORBIDDEN",
                       "The caller is not authorized to manage resources."));
        return;
    }

    try {
        resource_repository->remove_resource(
            haven::domain::OrganizationId{account.organization_id},
            haven::domain::ResourceId{resource_id});

        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k204NoContent);
        callback(response);
    } catch (const haven::application::RepositoryError&) {
        HVN_ERROR_LOG("Resource deletion failed");
        callback(error(request, drogon::k503ServiceUnavailable, "SERVICE_UNAVAILABLE",
                       "Resource deletion is temporarily unavailable."));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Resource deletion request failed unexpectedly");
        callback(error(request, drogon::k500InternalServerError, "INTERNAL_ERROR",
                       "An unexpected error occurred."));
    }
}

}  // namespace

void register_delete_resource_route(
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    drogon::app().registerHandler(
        kRoute,
        [resource_repository = std::move(resource_repository),
         authentication = std::move(authentication)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& resource_id) mutable {
            handle(resource_repository, authentication, request, std::move(callback), resource_id);
        },
        {drogon::Delete});
}

}  // namespace haven::presentation::admin
