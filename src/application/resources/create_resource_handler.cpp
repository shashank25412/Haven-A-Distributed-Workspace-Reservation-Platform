/**
 * @file create_resource_handler.cpp
 * @brief Implements the CreateResource application use case.
 */

#include "haven/application/resources/create_resource_handler.hpp"

#include "haven/domain/resource.hpp"
#include "haven/logging/logging.hpp"

namespace haven::application::resources {

CreateResourceHandler::CreateResourceHandler(
    ResourceRepository& resource_repository) noexcept
    : resource_repository_(resource_repository) {}

haven::domain::Resource CreateResourceHandler::handle(
    const CreateResourceCommand& command) const {
    HVN_TRACE_SCOPE();

    auto resource = haven::domain::Resource::create(
        command.organization_id(),
        command.resource_id(),
        command.name(),
        command.description(),
        command.resource_type(),
        command.requires_approval(),
        command.total_units(),
        command.address());

    resource_repository_.save(resource);

    return resource;
}

}  // namespace haven::application::resources
