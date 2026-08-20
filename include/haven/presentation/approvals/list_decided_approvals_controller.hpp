/**
 * @file list_decided_approvals_controller.hpp
 * @brief Declares route registration for listing decided reservation approvals.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/list_decided_approvals_handler.hpp"
#include "haven/application/resources/resource_repository.hpp"

#include <memory>

namespace haven::presentation::approvals {

void register_list_decided_approvals_route(
    std::shared_ptr<haven::application::reservations::ListDecidedApprovalsHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication);

}  // namespace haven::presentation::approvals
