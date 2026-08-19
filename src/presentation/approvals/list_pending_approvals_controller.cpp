/**
 * @file list_pending_approvals_controller.cpp
 * @brief Implements the pending reservation approvals listing HTTP route.
 */

#include "haven/presentation/approvals/list_pending_approvals_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/application/reservations/list_pending_approvals_query.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"
#include "haven/presentation/reservations/create_reservation_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <array>
#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::approvals {
namespace {

constexpr char kRoute[]{"/api/v1/approvals"};

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

bool has_approver_role(const std::string& role) {
    constexpr std::array<std::string_view, 4> approver_roles{
        "APPROVER", "RESOURCE_ADMIN", "ORG_ADMIN", "SYSTEM_ADMIN"};
    return std::any_of(approver_roles.begin(), approver_roles.end(), [&role](const auto candidate) {
        return role == candidate;
    });
}

void handle(
    const std::shared_ptr<haven::application::reservations::ListPendingApprovalsHandler>& handler,
    const std::shared_ptr<haven::application::resources::ResourceRepository>& resource_repository,
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
    } catch (const haven::application::auth::AuthenticationError& authentication_error) {
        const auto unavailable = authentication_error.code() ==
                                 haven::application::auth::AuthenticationErrorCode::persistence;
        callback(error(request,
                       unavailable ? drogon::k503ServiceUnavailable : drogon::k401Unauthorized,
                       unavailable ? "AUTHENTICATION_UNAVAILABLE" : "INVALID_SESSION",
                       unavailable ? "Authentication is temporarily unavailable."
                                   : "The authentication session is invalid or expired."));
        return;
    }

    if (!has_approver_role(account.role)) {
        callback(error(request,
                       drogon::k403Forbidden,
                       "FORBIDDEN",
                       "The caller is not authorized to review approvals."));
        return;
    }

    try {
        const auto organization_id = haven::domain::OrganizationId{account.organization_id};
        const auto query = haven::application::reservations::ListPendingApprovalsQuery{organization_id};
        const auto reservations = handler->handle(query);

        Json::Value body;
        body["items"] = Json::arrayValue;
        for (const auto& reservation : reservations) {
            Json::Value item;
            item["reservationId"] = reservation.reservation_id().value();

            Json::Value resource_reference;
            resource_reference["resourceId"] = reservation.resource_id().value();
            const auto resource =
                resource_repository->find_by_id(organization_id, reservation.resource_id());
            if (resource.has_value()) {
                resource_reference["name"] = resource->aggregate().name();
                resource_reference["resourceType"] =
                    std::string{haven::domain::to_string(resource->aggregate().type())};
            } else {
                resource_reference["name"] = reservation.resource_id().value();
                resource_reference["resourceType"] = Json::Value::null;
            }
            item["resource"] = std::move(resource_reference);

            Json::Value creator;
            creator["userId"] = reservation.created_by().value();
            // No user directory exists yet; the caller identifier doubles as the display name.
            creator["displayName"] = reservation.created_by().value();
            item["creator"] = std::move(creator);

            item["startTime"] = reservation_http_timestamp(reservation.interval().start());
            item["endTime"] = reservation_http_timestamp(reservation.interval().end());
            item["purpose"] = reservation.purpose().value();
            item["status"] = "PENDING_APPROVAL";
            body["items"].append(std::move(item));
        }
        body["pagination"]["page"] = 1;
        body["pagination"]["pageSize"] = static_cast<int>(reservations.size());
        body["pagination"]["totalItems"] = Json::UInt64{reservations.size()};
        body["pagination"]["totalPages"] = 1;

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    } catch (const haven::application::RepositoryError& repository_error) {
        using haven::application::RepositoryErrorCode;
        const auto unavailable = repository_error.code() == RepositoryErrorCode::Timeout ||
                                 repository_error.code() == RepositoryErrorCode::Authentication ||
                                 repository_error.code() == RepositoryErrorCode::Authorization;
        callback(
            error(request,
                  unavailable ? drogon::k503ServiceUnavailable : drogon::k500InternalServerError,
                  unavailable ? "SERVICE_UNAVAILABLE" : "INTERNAL_ERROR",
                  unavailable ? "A required service is temporarily unavailable."
                              : "The request could not be completed."));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Pending approvals listing request failed unexpectedly");
        callback(error(request,
                       drogon::k500InternalServerError,
                       "INTERNAL_ERROR",
                       "The request could not be completed."));
    } catch (...) {
        callback(error(request,
                       drogon::k500InternalServerError,
                       "INTERNAL_ERROR",
                       "The request could not be completed."));
    }
}

}  // namespace

void register_list_pending_approvals_route(
    std::shared_ptr<haven::application::reservations::ListPendingApprovalsHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    if (!handler)
        throw std::invalid_argument("List Pending Approvals route handler must not be null");
    if (!resource_repository)
        throw std::invalid_argument("List Pending Approvals resource repository must not be null");
    if (!authentication)
        throw std::invalid_argument("List Pending Approvals authentication service must not be null");

    drogon::app().registerHandler(
        kRoute,
        [handler = std::move(handler),
         resource_repository = std::move(resource_repository),
         authentication = std::move(authentication)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            handle(handler, resource_repository, authentication, request, std::move(callback));
        },
        {drogon::Get});
}

}  // namespace haven::presentation::approvals
