/**
 * @file organization_response.hpp
 * @brief Declares the public response model for organization directory entries.
 */

#pragma once

#include "haven/domain/organization.hpp"

#include <json/value.h>

namespace haven::presentation::organizations {

/**
 * @brief Converts an Organization aggregate into its public API representation.
 */
class OrganizationResponse final {
public:
    explicit OrganizationResponse(const haven::domain::Organization& organization);

    [[nodiscard]] Json::Value to_json() const;

private:
    Json::Value response_;
};

}  // namespace haven::presentation::organizations
