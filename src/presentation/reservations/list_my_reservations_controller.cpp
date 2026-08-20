/**
 * @file list_my_reservations_controller.cpp
 * @brief Implements the caller reservation listing HTTP route.
 */

#include "haven/presentation/reservations/list_my_reservations_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/application/reservations/list_caller_reservations_query.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"
#include "haven/presentation/reservations/create_reservation_request.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::reservations {
namespace {

constexpr char kRoute[]{"/api/v1/reservations/me"};

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

Json::Value resource_reference(
    const haven::application::resources::ResourceRepository& resource_repository,
    const haven::domain::OrganizationId& organization_id,
    const haven::domain::ResourceId& resource_id) {
    Json::Value reference;
    reference["resourceId"] = resource_id.value();
    const auto resource = resource_repository.find_by_id(organization_id, resource_id);
    if (resource.has_value()) {
        reference["name"] = resource->aggregate().name();
        reference["resourceType"] = std::string{haven::domain::to_string(resource->aggregate().type())};
    } else {
        // Resource metadata may no longer be visible; fall back to identifiers only.
        reference["name"] = resource_id.value();
        reference["resourceType"] = Json::Value::null;
    }
    return reference;
}

void handle(
    const std::shared_ptr<haven::application::reservations::ListCallerReservationsHandler>& handler,
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

    try {
        const auto organization_id = haven::domain::OrganizationId{account.organization_id};
        const auto query = haven::application::reservations::ListCallerReservationsQuery{
            organization_id, haven::domain::UserId{account.user_id}};
        const auto reservations = handler->handle(query);

        Json::Value body;
        body["items"] = Json::arrayValue;
        for (const auto& reservation : reservations) {
            Json::Value item;
            item["reservationId"] = reservation.reservation_id().value();
            item["resource"] =
                resource_reference(*resource_repository, organization_id, reservation.resource_id());
            item["startTime"] = reservation_http_timestamp(reservation.interval().start());
            item["endTime"] = reservation_http_timestamp(reservation.interval().end());
            item["purpose"] = reservation.purpose().value();
            item["status"] = std::string{haven::domain::to_string(reservation.status())};
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
        HVN_ERROR_LOG("Caller reservation listing request failed unexpectedly");
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

void register_list_my_reservations_route(
    std::shared_ptr<haven::application::reservations::ListCallerReservationsHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    if (!handler)
        throw std::invalid_argument("List My Reservations route handler must not be null");
    if (!resource_repository)
        throw std::invalid_argument("List My Reservations resource repository must not be null");
    if (!authentication)
        throw std::invalid_argument("List My Reservations authentication service must not be null");

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

}  // namespace haven::presentation::reservations
