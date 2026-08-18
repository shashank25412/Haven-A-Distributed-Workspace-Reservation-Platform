#include "haven/presentation/resources/search_resources_controller.hpp"

#include "haven/application/repository_error.hpp"
#include "haven/application/resources/search_available_resources_query.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/resource_type.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/http_timestamp.hpp"
#include "haven/presentation/resources/resource_error_response.hpp"
#include "haven/presentation/resources/resource_response.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::resources {
namespace {

[[nodiscard]] std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

[[nodiscard]] drogon::HttpResponsePtr error_response(const drogon::HttpRequestPtr& request,
                                                     const drogon::HttpStatusCode status,
                                                     std::string code,
                                                     std::string message) {
    auto request_id = request->getHeader("X-Request-Id");
    const auto body = ResourceErrorResponse{
        std::move(code), std::move(message), request_id.empty() ? "unavailable" : request_id};
    auto response = drogon::HttpResponse::newHttpJsonResponse(body.to_json());
    response->setStatusCode(status);
    response->setContentTypeString("application/problem+json");
    return response;
}

void handle_search(
    const std::shared_ptr<haven::application::resources::SearchAvailableResourcesHandler>& handler,
    const std::string& organization_id,
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto resource_type = request->getParameter("resourceType");
    const auto start_time = request->getParameter("startTime");
    const auto end_time = request->getParameter("endTime");
    const auto query_text = request->getParameter("query");
    if (resource_type.empty() || start_time.empty() || end_time.empty() || query_text.size() > 100U) {
        callback(error_response(request,
                                drogon::k400BadRequest,
                                "INVALID_REQUEST",
                                "Resource type and a valid time interval are required."));
        return;
    }

    try {
        const auto query = haven::application::resources::SearchAvailableResourcesQuery{
            haven::domain::OrganizationId{organization_id},
            haven::domain::resource_type_from_string(resource_type),
            haven::domain::TimeInterval{haven::presentation::parse_http_timestamp(start_time),
                                        haven::presentation::parse_http_timestamp(end_time)}};
        auto resources = handler->handle(query);
        const auto normalized_query = lowercase(query_text);
        if (!normalized_query.empty()) {
            std::erase_if(resources, [&normalized_query](const haven::domain::Resource& resource) {
                return lowercase(resource.name()).find(normalized_query) == std::string::npos &&
                       lowercase(resource.description()).find(normalized_query) == std::string::npos;
            });
        }
        std::sort(resources.begin(), resources.end(), [](const auto& left, const auto& right) {
            return left.name() < right.name();
        });

        Json::Value body;
        body["items"] = Json::arrayValue;
        for (const auto& resource : resources) {
            body["items"].append(ResourceResponse{resource}.to_json());
        }
        body["searchContext"]["startTime"] = start_time;
        body["searchContext"]["endTime"] = end_time;
        body["pagination"]["page"] = 1;
        body["pagination"]["pageSize"] = 100;
        body["pagination"]["totalItems"] = Json::UInt64{resources.size()};
        body["pagination"]["totalPages"] = resources.empty() ? 0 : 1;
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k200OK);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    } catch (const std::invalid_argument&) {
        callback(error_response(request,
                                drogon::k400BadRequest,
                                "INVALID_REQUEST",
                                "The resource search criteria are invalid."));
    } catch (const haven::application::RepositoryError& repository_error) {
        using haven::application::RepositoryErrorCode;
        const auto unavailable = repository_error.code() == RepositoryErrorCode::Timeout ||
                                 repository_error.code() == RepositoryErrorCode::Authentication ||
                                 repository_error.code() == RepositoryErrorCode::Authorization;
        callback(error_response(request,
                                unavailable ? drogon::k503ServiceUnavailable
                                            : drogon::k500InternalServerError,
                                unavailable ? "SERVICE_UNAVAILABLE" : "INTERNAL_ERROR",
                                unavailable ? "Resource search is temporarily unavailable."
                                            : "The resource search could not be completed."));
    } catch (const std::exception&) {
        HVN_ERROR_LOG("Resource search request failed unexpectedly");
        callback(error_response(request,
                                drogon::k500InternalServerError,
                                "INTERNAL_ERROR",
                                "The resource search could not be completed."));
    }
}

}  // namespace

void register_search_resources_route(
    std::shared_ptr<haven::application::resources::SearchAvailableResourcesHandler> handler,
    std::string public_organization_id) {
    if (!handler || public_organization_id.empty()) {
        throw std::invalid_argument("Resource search route configuration is invalid");
    }
    drogon::app().registerHandler(
        "/api/v1/resources",
        [handler = std::move(handler), organization_id = std::move(public_organization_id)](
            const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            handle_search(handler, organization_id, request, std::move(callback));
        },
        {drogon::Get});
}

}  // namespace haven::presentation::resources
