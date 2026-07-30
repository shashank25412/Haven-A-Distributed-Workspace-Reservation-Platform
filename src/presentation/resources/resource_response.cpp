/**
 * @file resource_response.cpp
 * @brief Implements Resource detail response serialization.
 */

#include "haven/presentation/resources/resource_response.hpp"

#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"

#include <json/value.h>

#include <string>

namespace haven::presentation::resources {

ResourceResponse::ResourceResponse(const haven::domain::Resource& resource) {
    response_["organizationId"] = resource.organization_id().value();
    response_["resourceId"] = resource.resource_id().value();
    response_["name"] = resource.name();
    response_["description"] = resource.description();
    response_["resourceType"] = std::string{haven::domain::to_string(resource.type())};
    response_["status"] = std::string{haven::domain::to_string(resource.status())};
    response_["requiresApproval"] = resource.requires_approval();
    response_["version"] = Json::UInt64{resource.version().value()};
}

Json::Value ResourceResponse::to_json() const {
    return response_;
}

}  // namespace haven::presentation::resources
