/**
 * @file resource_response.hpp
 * @brief Declares the public response model for Resource detail requests.
 */

#pragma once

#include "haven/domain/resource.hpp"

#include <json/value.h>

namespace haven::presentation::resources {

/**
 * @brief Converts a Resource aggregate into its public API representation.
 */
class ResourceResponse final {
public:
    /**
     * @brief Constructs a response from an application result.
     *
     * @param resource Resource aggregate returned by the application layer.
     */
    explicit ResourceResponse(const haven::domain::Resource& resource);

    /**
     * @brief Serializes every public Resource detail field.
     *
     * @return JSON matching the OpenAPI ResourceDetails schema.
     */
    [[nodiscard]] Json::Value to_json() const;

private:
    Json::Value response_;
};

}  // namespace haven::presentation::resources
