#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/resources/search_available_resources_handler.hpp"

#include <memory>
#include <string>

namespace haven::presentation::resources {

/**
 * @brief Registers the resource search route.
 *
 * A caller may request a specific organization via the `organizationId`
 * query parameter. Admin-role callers are always locked to their own
 * organization regardless of the supplied parameter; other authenticated and
 * anonymous callers may browse any organization's resource catalog.
 *
 * @param handler Application handler retained for the route callback lifetime.
 * @param public_organization_id Fallback organization used when neither an
 * authenticated caller nor an `organizationId` query parameter is present.
 * @param authentication Optional authentication service used to resolve the
 * caller's role and organization when a bearer session is supplied.
 */
void register_search_resources_route(
    std::shared_ptr<haven::application::resources::SearchAvailableResourcesHandler> handler,
    std::string public_organization_id,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::resources
