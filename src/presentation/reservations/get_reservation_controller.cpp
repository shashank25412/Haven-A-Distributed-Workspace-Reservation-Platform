/**
 * @file get_reservation_controller.cpp
 * @brief Implements the caller-owned single reservation retrieval HTTP route.
 */

#include "haven/presentation/reservations/get_reservation_controller.hpp"

#include "haven/application/reservations/get_reservation_query.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
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

constexpr char kRoute[]{"/api/v1/reservations/{reservationId}"};
constexpr std::size_t kMaximumPathIdentifierLength{255};

bool is_invalid_identifier(const std::string& value) {
    return value.empty() || value.size() > kMaximumPathIdentifierLength ||
           std::any_of(value.begin(), value.end(), [](const unsigned char character) {
               return std::iscntrl(character) != 0 || std::isspace(character) != 0;
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
        reference["address"] = resource->aggregate().address();
    } else {
        // Resource metadata may no longer be visible; fall back to identifiers only.
        reference["name"] = resource_id.value();
        reference["resourceType"] = Json::Value::null;
        reference["address"] = "";
    }
    return reference;
}

void handle(
    const std::shared_ptr<haven::application::reservations::GetReservationHandler>& handler,
    const std::shared_ptr<haven::application::resources::ResourceRepository>& resource_repository,
    const std::shared_ptr<haven::application::auth::AuthenticationService>& authentication,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string reservation_id) {
    HVN_TRACE_SCOPE();

    if (is_invalid_identifier(reservation_id)) {
        callback(error(request,
                       drogon::k400BadRequest,
                       "INVALID_REQUEST",
                       "The reservation identifier is invalid."));
        return;
    }

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
        const auto query = haven::application::reservations::GetReservationQuery{
            organization_id, haven::domain::ReservationId{reservation_id}};
        const auto reservation = handler->handle(query);

        // A reservation owned by another caller is reported the same way as a missing one, so
        // its existence can't be inferred by callers who aren't its owner.
        if (!reservation.has_value() ||
            reservation->created_by() != haven::domain::UserId{account.user_id}) {
            callback(error(request,
                           drogon::k404NotFound,
                           "RESERVATION_NOT_FOUND",
                           "The reservation was not found."));
            return;
        }

        Json::Value body;
        body["reservationId"] = reservation->reservation_id().value();
        body["resource"] =
            resource_reference(*resource_repository, organization_id, reservation->resource_id());
        body["startTime"] = reservation_http_timestamp(reservation->interval().start());
        body["endTime"] = reservation_http_timestamp(reservation->interval().end());
        body["purpose"] = reservation->purpose().value();
        body["status"] = std::string{haven::domain::to_string(reservation->status())};
        if (reservation->cancellation_info().has_value() &&
            reservation->cancellation_info()->reason().has_value()) {
            body["cancellationReason"] = *reservation->cancellation_info()->reason();
        }

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Get reservation request failed unexpectedly");
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

void register_get_reservation_route(
    std::shared_ptr<haven::application::reservations::GetReservationHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    if (!handler)
        throw std::invalid_argument("Get Reservation route handler must not be null");
    if (!resource_repository)
        throw std::invalid_argument("Get Reservation resource repository must not be null");
    if (!authentication)
        throw std::invalid_argument("Get Reservation authentication service must not be null");

    drogon::app().registerHandler(
        kRoute,
        [handler = std::move(handler),
         resource_repository = std::move(resource_repository),
         authentication = std::move(authentication)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            std::string reservation_id) {
            handle(handler, resource_repository, authentication, request, std::move(callback),
                   std::move(reservation_id));
        },
        {drogon::Get});
}

}  // namespace haven::presentation::reservations
