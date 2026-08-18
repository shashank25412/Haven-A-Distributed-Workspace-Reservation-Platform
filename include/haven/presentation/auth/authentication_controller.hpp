#pragma once

#include "haven/application/auth/authentication_service.hpp"

#include <memory>

namespace haven::presentation::auth {

void register_authentication_routes(
    std::shared_ptr<haven::application::auth::AuthenticationService> service);

}  // namespace haven::presentation::auth
