/**
 * @file organization_response.cpp
 * @brief Implements the public response model for organization directory entries.
 */

#include "haven/presentation/organizations/organization_response.hpp"

namespace haven::presentation::organizations {

OrganizationResponse::OrganizationResponse(const haven::domain::Organization& organization) {
    response_["organizationId"] = organization.organization_id().value();
    response_["name"] = organization.name();
    if (!organization.image_url().empty()) {
        response_["imageUrl"] = organization.image_url();
    }
}

Json::Value OrganizationResponse::to_json() const {
    return response_;
}

}  // namespace haven::presentation::organizations
