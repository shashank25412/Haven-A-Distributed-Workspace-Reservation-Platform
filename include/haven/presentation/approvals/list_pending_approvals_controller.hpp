/**
 * @file list_pending_approvals_controller.hpp
 * @brief Declares route registration for listing pending reservation approvals.
 */

#pragma once

#include "haven/application/auth/authentication_service.hpp"
#include "haven/application/reservations/list_pending_approvals_handler.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/application/users/user_directory.hpp"

#include <memory>

namespace haven::presentation::approvals {

void register_list_pending_approvals_route(
    std::shared_ptr<haven::application::reservations::ListPendingApprovalsHandler> handler,
    std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository,
    std::shared_ptr<haven::application::auth::AuthenticationService> authentication,
    std::shared_ptr<haven::application::users::UserDirectory> user_directory);

}  // namespace haven::presentation::approvals
