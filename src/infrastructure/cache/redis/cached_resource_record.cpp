#include "haven/infrastructure/cache/redis/cached_resource_record.hpp"

#include "haven/domain/value_objects/resource_status.hpp"
#include "haven/domain/value_objects/resource_type.hpp"

#include <json/json.h>
#include <sstream>
#include <stdexcept>

namespace haven::infrastructure::cache::redis {

std::string serialize_cached_resource(const haven::domain::Resource& resource) {
    Json::Value json;
    json["schemaVersion"] = Json::UInt64{1U};
    json["organizationId"] = resource.organization_id().value();
    json["resourceId"] = resource.resource_id().value();
    json["name"] = resource.name();
    json["description"] = resource.description();
    json["resourceType"] = std::string{haven::domain::to_string(resource.type())};
    json["status"] = std::string{haven::domain::to_string(resource.status())};
    json["requiresApproval"] = resource.requires_approval();
    json["version"] = Json::UInt64{resource.version().value()};
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return Json::writeString(writer, json);
}

haven::domain::Resource deserialize_cached_resource(const std::string& payload) {
    Json::Value json;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream input{payload};
    if (!Json::parseFromStream(reader, input, &json, &errors)) {
        throw std::invalid_argument{"Malformed Resource cache JSON"};
    }
    if (json["schemaVersion"].asUInt64() != 1U) {
        throw std::invalid_argument{"Unsupported Resource cache schema"};
    }
    return haven::domain::Resource::rehydrate(
        haven::domain::OrganizationId{json["organizationId"].asString()},
        haven::domain::ResourceId{json["resourceId"].asString()},
        json["name"].asString(),
        json["description"].asString(),
        haven::domain::resource_type_from_string(json["resourceType"].asString()),
        haven::domain::resource_status_from_string(json["status"].asString()),
        json["requiresApproval"].asBool(),
        haven::domain::Version{json["version"].asUInt64()});
}

}  // namespace haven::infrastructure::cache::redis
