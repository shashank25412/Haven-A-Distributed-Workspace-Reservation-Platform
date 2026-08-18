#include "haven/presentation/auth/authentication_controller.hpp"

#include "haven/logging/logging.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <json/value.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace haven::presentation::auth {
namespace {

using AuthenticationError =
    haven::application::auth::AuthenticationError;
using AuthenticationErrorCode =
    haven::application::auth::AuthenticationErrorCode;
using AuthenticationService =
    haven::application::auth::AuthenticationService;
using AuthenticatedAccount =
    haven::application::auth::AuthenticatedAccount;

[[nodiscard]] drogon::HttpResponsePtr json_error(const drogon::HttpStatusCode status,
                                                 std::string code,
                                                 std::string message) {
    Json::Value body;
    body["code"] = std::move(code);
    body["message"] = std::move(message);
    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(status);
    response->setContentTypeString("application/problem+json");
    return response;
}

[[nodiscard]] drogon::HttpResponsePtr authentication_error(const AuthenticationError& error) {
    switch (error.code()) {
        case AuthenticationErrorCode::invalid_input:
            return json_error(drogon::k400BadRequest, "INVALID_REQUEST", error.what());
        case AuthenticationErrorCode::account_not_found:
            return json_error(drogon::k404NotFound, "ACCOUNT_NOT_FOUND", error.what());
        case AuthenticationErrorCode::account_exists:
            return json_error(drogon::k409Conflict, "ACCOUNT_EXISTS", error.what());
        case AuthenticationErrorCode::invalid_credentials:
        case AuthenticationErrorCode::invalid_session:
            return json_error(drogon::k401Unauthorized, "INVALID_CREDENTIALS", error.what());
        case AuthenticationErrorCode::persistence:
            return json_error(drogon::k503ServiceUnavailable,
                              "AUTHENTICATION_UNAVAILABLE",
                              "Authentication is temporarily unavailable.");
    }
    return json_error(drogon::k500InternalServerError,
                      "INTERNAL_ERROR",
                      "Authentication could not be completed.");
}

[[nodiscard]] Json::Value account_response(const AuthenticatedAccount& account) {
    Json::Value body;
    body["accessToken"] = account.access_token;
    body["tokenType"] = "Bearer";
    body["user"]["userId"] = account.user_id;
    body["user"]["email"] = account.email;
    body["user"]["organizationId"] = account.organization_id;
    body["user"]["role"] = account.role;
    return body;
}

struct Credentials final {
    std::string email;
    std::string password;
};

[[nodiscard]] std::optional<Credentials> credentials_from(
    const drogon::HttpRequestPtr& request) {
    const auto json = request->getJsonObject();
    if (!json || !json->isObject() || !json->isMember("email") ||
        !(*json)["email"].isString() || !json->isMember("password") ||
        !(*json)["password"].isString()) {
        return std::nullopt;
    }
    return Credentials{.email = (*json)["email"].asString(),
                       .password = (*json)["password"].asString()};
}

[[nodiscard]] std::optional<std::string> bearer_token(const drogon::HttpRequestPtr& request) {
    constexpr std::string_view prefix{"Bearer "};
    const auto authorization = request->getHeader("Authorization");
    if (!authorization.starts_with(prefix) || authorization.size() <= prefix.size()) {
        return std::nullopt;
    }
    return authorization.substr(prefix.size());
}

template <typename Operation>
void handle_credentials(const std::shared_ptr<AuthenticationService>& service,
                        const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>& callback,
                        const drogon::HttpStatusCode success_status,
                        Operation&& operation) {
    const auto credentials = credentials_from(request);
    if (!credentials) {
        callback(json_error(drogon::k400BadRequest,
                            "INVALID_REQUEST",
                            "Email and password are required."));
        return;
    }
    try {
        const auto account = operation(*service, credentials->email, credentials->password);
        auto response = drogon::HttpResponse::newHttpJsonResponse(account_response(account));
        response->setStatusCode(success_status);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    } catch (const AuthenticationError& error) {
        callback(authentication_error(error));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Authentication request failed unexpectedly");
        callback(json_error(drogon::k500InternalServerError,
                            "INTERNAL_ERROR",
                            "Authentication could not be completed."));
    }
}

}  // namespace

void register_authentication_routes(std::shared_ptr<AuthenticationService> service) {
    if (!service) throw std::invalid_argument("Authentication route service must not be null");

    drogon::app().registerHandler(
        "/api/v1/auth/signup",
        [service](const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            handle_credentials(service,
                               request,
                               callback,
                               drogon::k201Created,
                               [](const AuthenticationService& authentication,
                                  const std::string_view email,
                                  const std::string_view password) {
                                   return authentication.sign_up(email, password);
                               });
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/v1/auth/login",
        [service](const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            handle_credentials(service,
                               request,
                               callback,
                               drogon::k200OK,
                               [](const AuthenticationService& authentication,
                                  const std::string_view email,
                                  const std::string_view password) {
                                   return authentication.login(email, password);
                               });
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/v1/auth/session",
        [service](const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            const auto token = bearer_token(request);
            if (!token) {
                callback(json_error(drogon::k401Unauthorized,
                                    "INVALID_SESSION",
                                    "A bearer session is required."));
                return;
            }
            if (request->method() == drogon::Delete) {
                service->logout(*token);
                auto response = drogon::HttpResponse::newHttpResponse();
                response->setStatusCode(drogon::k204NoContent);
                callback(response);
                return;
            }
            try {
                const auto account = service->authenticate(*token);
                auto response = drogon::HttpResponse::newHttpJsonResponse(account_response(account));
                response->addHeader("Cache-Control", "no-store");
                callback(response);
            } catch (const AuthenticationError& error) {
                callback(authentication_error(error));
            }
        },
        {drogon::Get, drogon::Delete});
}

}  // namespace haven::presentation::auth
