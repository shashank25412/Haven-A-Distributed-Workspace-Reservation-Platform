/**
 * @file admin_cancel_reservation_controller.cpp
 * @brief Implements the administrative reservation cancellation HTTP route.
 */

#include "haven/presentation/admin/admin_cancel_reservation_controller.hpp"

#include "haven/application/reservations/cancel_reservation_command.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"
#include "haven/presentation/reservations/create_reservation_request.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace haven::presentation::admin {
namespace {

constexpr char kRoute[]{"/api/v1/admin/reservations/{reservationId}/cancel"};
constexpr std::size_t kMaximumPathIdentifierLength{255};

bool has_admin_role(const std::string& role) {
    constexpr std::array<std::string_view, 4> admin_roles{
        "APPROVER", "RESOURCE_ADMIN", "ORG_ADMIN", "SYSTEM_ADMIN"};
    return std::any_of(admin_roles.begin(), admin_roles.end(), [&role](const auto candidate) {
        return role == candidate;
    });
}

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

std::string cancellation_comment(const drogon::HttpRequestPtr& request) {
    const auto* json = request->getJsonObject().get();
    if (json == nullptr || !json->isMember("comment") || !(*json)["comment"].isString()) {
        return {};
    }
    return (*json)["comment"].asString();
}

void handle(
    const std::shared_ptr<haven::application::reservations::CancelReservationHandler>& handler,
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

    if (!has_admin_role(account.role)) {
        callback(error(request,
                       drogon::k403Forbidden,
                       "FORBIDDEN",
                       "The caller is not authorized to cancel other members' reservations."));
        return;
    }

    try {
        const auto uuid = [] { return drogon::utils::getUuid(true); };
        const auto comment = cancellation_comment(request);
        const haven::application::reservations::CancelReservationCommand command{
            haven::domain::OrganizationId{account.organization_id},
            haven::domain::ReservationId{reservation_id},
            haven::domain::UserId{account.user_id},
            haven::domain::EventId{uuid()},
            std::chrono::system_clock::now(),
            /*bypass_owner_check=*/true,
            comment.empty() ? std::nullopt : std::optional<std::string>{comment}};

        const auto result = handler->handle(command);
        using enum haven::application::reservations::CancelReservationStatus;
        switch (result.status()) {
            case CANCELLED: {
                const auto& reservation = *result.reservation();
                Json::Value body;
                body["reservationId"] = reservation.reservation_id().value();
                body["status"] = std::string{haven::domain::to_string(reservation.status())};
                body["comment"] = comment.empty() ? Json::Value::null : Json::Value{comment};
                callback(drogon::HttpResponse::newHttpJsonResponse(body));
                return;
            }
            case RESERVATION_NOT_FOUND:
                callback(error(request,
                               drogon::k404NotFound,
                               "RESERVATION_NOT_FOUND",
                               "The reservation was not found."));
                return;
            case CALLER_NOT_AUTHORIZED:
                callback(error(request,
                               drogon::k403Forbidden,
                               "FORBIDDEN",
                               "The caller is not authorized to cancel this reservation."));
                return;
            case INVALID_STATE:
                callback(error(request,
                               drogon::k409Conflict,
                               "INVALID_RESERVATION_STATE",
                               "The reservation has already been completed or is otherwise no "
                               "longer cancellable."));
                return;
        }
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Administrative reservation cancellation request failed unexpectedly");
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

void register_admin_cancel_reservation_route(
    std::shared_ptr<haven::application::reservations::CancelReservationHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    if (!handler)
        throw std::invalid_argument("Admin Cancel Reservation route handler must not be null");
    if (!authentication)
        throw std::invalid_argument("Admin Cancel Reservation authentication service must not be null");

    drogon::app().registerHandler(
        kRoute,
        [handler = std::move(handler), authentication = std::move(authentication)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            std::string reservation_id) {
            handle(handler, authentication, request, std::move(callback), std::move(reservation_id));
        },
        {drogon::Post});
}

}  // namespace haven::presentation::admin
