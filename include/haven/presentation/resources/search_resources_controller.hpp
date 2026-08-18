#pragma once

#include "haven/application/resources/search_available_resources_handler.hpp"

#include <memory>
#include <string>

namespace haven::presentation::resources {

void register_search_resources_route(
    std::shared_ptr<haven::application::resources::SearchAvailableResourcesHandler> handler,
    std::string public_organization_id);

}  // namespace haven::presentation::resources
