/**
 * @file bulk_create_resources_controller.cpp
 * @brief Implements the admin bulk resource creation HTTP route.
 *
 * Expects POST /api/v1/admin/resources/bulk with body:
 *   { "items": [ { "name", "description", "resourceType", "requiresApproval",
 *                  "totalUnits", "address" }, ... ] }
 *
 * The client parses any spreadsheet/CSV locally and sends clean JSON so the
 * server needs no file-format parser dependency.
 */

#include "haven/presentation/admin/bulk_create_resources_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/api_error_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>
#include <json/value.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace haven::presentation::admin {
namespace {

constexpr char kRoute[]{"/api/v1/admin/resources/bulk"};
constexpr std::size_t kMaxBulkItems{500};

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

    const auto* json = request->getJsonObject().get();
    if (json == nullptr || !json->isMember("items") || !(*json)["items"].isArray()) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST",
                       "Request body must contain an 'items' array."));
        return;
    }

    const auto& items = (*json)["items"];
    if (items.empty()) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST",
                       "The 'items' array must not be empty."));
        return;
    }
    if (items.size() > kMaxBulkItems) {
        callback(error(request, drogon::k400BadRequest, "INVALID_REQUEST",
                       "Bulk import is limited to " + std::to_string(kMaxBulkItems) + " items per request."));
        return;
    }

    const haven::domain::OrganizationId org_id{account.organization_id};
    std::uint32_t imported = 0;
    Json::Value row_errors{Json::arrayValue};

    for (Json::ArrayIndex i = 0; i < items.size(); ++i) {
        const auto& item = items[i];

        const auto row_label = "Row " + std::to_string(i + 1);

        const auto get_str = [&](const char* field) -> std::optional<std::string> {
            if (!item.isMember(field) || !item[field].isString()) return std::nullopt;
            auto v = item[field].asString();
            if (v.empty()) return std::nullopt;
            return v;
        };

        const auto name = get_str("name");
        if (!name) {
            row_errors.append(row_label + ": 'name' is required.");
            continue;
        }
        const auto resource_type_str = get_str("resourceType");
        if (!resource_type_str) {
            row_errors.append(row_label + ": 'resourceType' is required.");
            continue;
        }

        haven::domain::ResourceType resource_type;
        try {
            resource_type = haven::domain::resource_type_from_string(*resource_type_str);
        } catch (const std::invalid_argument&) {
            row_errors.append(row_label + ": unknown resourceType '" + *resource_type_str + "'.");
            continue;
        }

        const auto description = get_str("description").value_or("");
        const bool requires_approval =
            item.isMember("requiresApproval") && item["requiresApproval"].isBool()
                ? item["requiresApproval"].asBool()
                : false;
        const std::uint32_t total_units =
            item.isMember("totalUnits") && item["totalUnits"].isUInt()
                ? item["totalUnits"].asUInt()
                : 1U;
        const auto address = get_str("address").value_or("");

        try {
            const haven::application::resources::CreateResourceCommand command{
                org_id,
                haven::domain::ResourceId{drogon::utils::getUuid(true)},
                *name,
                description,
                resource_type,
                requires_approval,
                total_units,
                address};
            handler->handle(command);
            ++imported;
        } catch (const std::invalid_argument& ex) {
            row_errors.append(row_label + ": " + ex.what());
        } catch (const haven::application::RepositoryError& repo_err) {
            HVN_ERROR_LOG("Bulk resource creation row ", i + 1, " failed: ", repo_err.what());
            row_errors.append(row_label + ": persistence error — " + repo_err.what());
        } catch (const std::exception&) {
            HVN_ERROR_LOG("Bulk resource creation row ", i + 1, " failed unexpectedly");
            row_errors.append(row_label + ": unexpected error.");
        }
    }

    Json::Value body;
    body["imported"] = Json::UInt{imported};
    body["failed"] = Json::UInt{static_cast<unsigned>(row_errors.size())};
    body["errors"] = std::move(row_errors);

    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(drogon::k200OK);
    callback(response);
}

}  // namespace

void register_bulk_create_resources_route(
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
