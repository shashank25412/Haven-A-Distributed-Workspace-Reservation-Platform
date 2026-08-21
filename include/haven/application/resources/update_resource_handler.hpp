/**
 * @file update_resource_handler.hpp
 * @brief Declares the UpdateResource application use-case handler.
 */

#pragma once

#include "haven/application/resources/resource_repository.hpp"
#include "haven/application/resources/update_resource_command.hpp"
#include "haven/domain/resource.hpp"

#include <optional>

namespace haven::application::resources {

class UpdateResourceHandler final {
public:
    explicit UpdateResourceHandler(ResourceRepository& resource_repository) noexcept;

    /** @return The updated resource, or nullopt when the resource does not exist. */
    [[nodiscard]] std::optional<haven::domain::Resource> handle(
        const UpdateResourceCommand& command) const;

private:
    ResourceRepository& resource_repository_;
};

}  // namespace haven::application::resources
