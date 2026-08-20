/**
 * @file reject_reservation_controller.cpp
 * @brief Implements the reservation rejection HTTP route.
 */

#include "haven/presentation/approvals/reject_reservation_controller.hpp"

#include "haven/application/reservations/reject_reservation_command.hpp"
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
#include <chrono>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::approvals {
namespace {

constexpr char kRoute[]{"/api/v1/approvals/{reservationId}/reject"};
constexpr std::size_t kMaximumPathIdentifierLength{255};

bool has_approver_role(const std::string& role) {
    constexpr std::array<std::string_view, 4> approver_roles{
        "APPROVER", "RESOURCE_ADMIN", "ORG_ADMIN", "SYSTEM_ADMIN"};
    return std::any_of(approver_roles.begin(), approver_roles.end(), [&role](const auto candidate) {
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

std::string rejection_reason(const drogon::HttpRequestPtr& request) {
    const auto* json = request->getJsonObject().get();
    if (json == nullptr || !json->isMember("reason") || !(*json)["reason"].isString()) {
        return {};
    }
    return (*json)["reason"].asString();
}

void handle(
    const std::shared_ptr<haven::application::reservations::RejectReservationHandler>& handler,
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

    if (!has_approver_role(account.role)) {
        callback(error(request,
                       drogon::k403Forbidden,
                       "FORBIDDEN",
                       "The caller is not authorized to review approvals."));
        return;
    }

    const auto reason = rejection_reason(request);

    try {
        const auto uuid = [] { return drogon::utils::getUuid(true); };
        const haven::application::reservations::RejectReservationCommand command{
            haven::domain::OrganizationId{account.organization_id},
            haven::domain::ReservationId{reservation_id},
            haven::domain::UserId{account.user_id},
            haven::domain::EventId{uuid()},
            std::chrono::system_clock::now(),
            reason.empty() ? std::nullopt : std::optional<std::string>{reason}};

        const auto result = handler->handle(command);
        using enum haven::application::reservations::RejectReservationStatus;
        switch (result.status()) {
            case REJECTED: {
                const auto& reservation = *result.reservation();
                Json::Value body;
                body["reservationId"] = reservation.reservation_id().value();
                body["status"] = std::string{haven::domain::to_string(reservation.status())};
                const auto& rejection = reservation.rejection_info();
                body["rejectedAt"] = rejection.has_value()
                                          ? reservations::reservation_http_timestamp(rejection->rejected_at())
                                          : reservations::reservation_http_timestamp(std::chrono::system_clock::now());
                if (rejection.has_value() && rejection->reason().has_value()) {
                    body["reason"] = *rejection->reason();
                } else {
                    body["reason"] = Json::Value::null;
                }
                callback(drogon::HttpResponse::newHttpJsonResponse(body));
                return;
            }
            case RESERVATION_NOT_FOUND:
                callback(error(request,
                               drogon::k404NotFound,
                               "RESERVATION_NOT_FOUND",
                               "The reservation was not found."));
                return;
            case INVALID_STATE:
                callback(error(request,
                               drogon::k409Conflict,
                               "INVALID_RESERVATION_STATE",
                               "The reservation is not pending approval."));
                return;
        }
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Reservation rejection request failed unexpectedly");
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

void register_reject_reservation_route(
    std::shared_ptr<haven::application::reservations::RejectReservationHandler> handler,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication) {
    if (!handler)
        throw std::invalid_argument("Reject Reservation route handler must not be null");
    if (!authentication)
        throw std::invalid_argument("Reject Reservation authentication service must not be null");

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

}  // namespace haven::presentation::approvals
