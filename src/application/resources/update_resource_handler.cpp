/**
 * @file update_resource_handler.cpp
 * @brief Implements the UpdateResource application use case.
 */

#include "haven/application/resources/update_resource_handler.hpp"

#include "haven/domain/value_objects/version.hpp"
#include "haven/logging/logging.hpp"

#include <optional>

namespace haven::application::resources {

UpdateResourceHandler::UpdateResourceHandler(
    ResourceRepository& resource_repository) noexcept
    : resource_repository_(resource_repository) {}

std::optional<haven::domain::Resource> UpdateResourceHandler::handle(
    const UpdateResourceCommand& command) const {
    HVN_TRACE_SCOPE();

    const auto loaded = resource_repository_.find_by_id(
        command.organization_id(), command.resource_id());

    if (!loaded.has_value()) {
        return std::nullopt;
    }

    const auto& current = loaded->aggregate();

    // Increment the version to reflect the mutation.
    auto updated = haven::domain::Resource::rehydrate(
        command.organization_id(),
        command.resource_id(),
        command.name(),
        command.description(),
        command.resource_type(),
        command.status(),
        command.requires_approval(),
        haven::domain::Version{current.version().value() + 1},
        command.total_units(),
        command.address());

    resource_repository_.update_resource(updated);

    return updated;
}

}  // namespace haven::application::resources
