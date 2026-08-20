/**
 * @file list_decided_approvals_handler.hpp
 * @brief Declares the ListDecidedApprovals application use-case handler.
 */

#pragma once

#include "haven/application/reservations/list_decided_approvals_query.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Lists previously decided (approved or rejected) reservations within one organization.
 */
class ListDecidedApprovalsHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit ListDecidedApprovalsHandler(
        ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the tenant-scoped decision history query.
     *
     * @param query Organization used to scope the history.
     * @return Reservations previously approved or rejected.
     */
    [[nodiscard]] ReservationListResult handle(
        const ListDecidedApprovalsQuery& query) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations
