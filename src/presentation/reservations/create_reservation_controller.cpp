#include "haven/presentation/reservations/create_reservation_controller.hpp"

#include "haven/application/idempotency/idempotency_repository_error.hpp"
#include "haven/application/repository_error.hpp"
#include "haven/domain/value_objects/idempotency_key.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"
#include "haven/presentation/reservations/create_reservation_request.hpp"
#include "haven/presentation/reservations/create_reservation_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::reservations {
namespace {
constexpr char kRoute[]{"/api/v1/reservations"};
constexpr std::size_t kMaximumHeaderLength{255};

bool invalid_header(const std::string& value, const bool reject_whitespace) {
    return value.empty() || value.size() > kMaximumHeaderLength ||
           std::any_of(value.begin(),
                       value.end(),
                       [reject_whitespace](const unsigned char c) {
                           return std::iscntrl(c) != 0 ||
                                  (reject_whitespace && std::isspace(c) != 0);
                       }) ||
           (reject_whitespace && std::all_of(value.begin(), value.end(), [](const unsigned char c) {
                return std::isspace(c) != 0;
            }));
}

std::string trace_id(const drogon::HttpRequestPtr& request) {
    auto value = request->getHeader("X-Request-Id");
    return value.empty() ? "unavailable" : std::move(value);
}

drogon::HttpResponsePtr error(const drogon::HttpRequestPtr& request,
                              const drogon::HttpStatusCode status,
                              std::string code,
                              std::string message) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(haven::presentation::ApiErrorResponse{
        std::move(code), std::move(message), trace_id(request)}
                                                                  .to_json());
    response->setStatusCode(status);
    response->setContentTypeString("application/problem+json");
    return response;
}

void map_result(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>& callback,
                const haven::application::reservations::CreateReservationResult& result) {
    using enum haven::application::reservations::CreateReservationStatus;
    switch (result.status()) {
        case CREATED_CONFIRMED:
        case CREATED_PENDING_APPROVAL: {
            auto response = drogon::HttpResponse::newHttpJsonResponse(
                CreateReservationResponse{result}.to_json());
            response->setStatusCode(drogon::k201Created);
            response->addHeader(
                "Location",
                "/api/v1/reservations/" + result.reservation()->reservation_id().value());
            callback(response);
            return;
        }
        case RESOURCE_NOT_FOUND:
            callback(error(request,
                           drogon::k404NotFound,
                           "RESOURCE_NOT_FOUND",
                           "The selected resource was not found."));
            return;
        case RESOURCE_INACTIVE:
            callback(error(request,
                           drogon::k422UnprocessableEntity,
                           "RESOURCE_INACTIVE",
                           "The selected resource is not active."));
            return;
        case SCHEDULE_CONFLICT:
            callback(error(request,
                           drogon::k409Conflict,
                           "RESERVATION_CONFLICT",
                           "The resource is no longer available for the requested interval."));
            return;
        case POLICY_REJECTED:
            callback(error(request,
                           drogon::k422UnprocessableEntity,
                           "RESERVATION_POLICY_REJECTED",
                           "The reservation request violates policy."));
            return;
        case IDEMPOTENCY_CONFLICT:
            callback(error(request,
                           drogon::k409Conflict,
                           "IDEMPOTENCY_KEY_MISMATCH",
                           "The idempotency key was already used with a different request."));
            return;
        case IDEMPOTENCY_IN_PROGRESS: {
            auto response = error(request,
                                  drogon::k409Conflict,
                                  "IDEMPOTENCY_REQUEST_IN_PROGRESS",
                                  "The idempotent request is still processing.");
            response->addHeader("Retry-After", "1");
            callback(response);
            return;
        }
    }
}

void handle(
    const std::shared_ptr<haven::application::reservations::CreateReservationHandler>& handler,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    HVN_TRACE_SCOPE();
    const auto key = request->getHeader("Idempotency-Key");
    const auto organization = request->getHeader("X-Haven-Organization-Id");
    const auto user = request->getHeader("X-Haven-User-Id");
    if (invalid_header(key, false) ||
        std::all_of(
            key.begin(), key.end(), [](const unsigned char c) { return std::isspace(c) != 0; }) ||
        invalid_header(organization, true) || invalid_header(user, true)) {
        callback(error(request,
                       drogon::k400BadRequest,
                       "INVALID_REQUEST",
                       "Required headers are missing or invalid."));
        return;
    }
    std::optional<haven::application::reservations::CreateReservationCommand> command;
    try {
        const auto* json = request->getJsonObject().get();
        if (json == nullptr)
            throw std::invalid_argument("Missing JSON body");
        const auto parsed = CreateReservationRequest::from_json(*json);
        const auto now = std::chrono::system_clock::now();
        const auto uuid = [] { return drogon::utils::getUuid(true); };
        command.emplace(haven::domain::OrganizationId{organization},
                        haven::domain::IdempotencyKey{key},
                        haven::domain::ReservationId{uuid()},
                        parsed.resource_id(),
                        haven::domain::UserId{user},
                        parsed.interval(),
                        parsed.purpose(),
                        haven::domain::ReservationKind::Standard,
                        false,
                        haven::domain::EventId{uuid()},
                        haven::domain::EventId{uuid()},
                        haven::domain::EventId{uuid()},
                        now);
    } catch (const std::invalid_argument&) {
        callback(error(
            request, drogon::k400BadRequest, "INVALID_REQUEST", "The request body is invalid."));
        return;
    }
    try {
        map_result(request, callback, handler->handle(*command));
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
    } catch (const haven::application::idempotency::IdempotencyRepositoryError&) {
        callback(error(request,
                       drogon::k500InternalServerError,
                       "INTERNAL_ERROR",
                       "The request could not be completed."));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Reservation creation request failed unexpectedly");
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

void register_create_reservation_route(
    std::shared_ptr<haven::application::reservations::CreateReservationHandler> handler) {
    if (!handler)
        throw std::invalid_argument("Create Reservation route handler must not be null");
    drogon::app().registerHandler(
        kRoute,
        [handler = std::move(handler)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            handle(handler, request, std::move(callback));
        },
        {drogon::Post});
}

}  // namespace haven::presentation::reservations
