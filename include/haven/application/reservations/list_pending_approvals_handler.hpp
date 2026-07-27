/**
 * @file list_pending_approvals_handler.hpp
 * @brief Declares the ListPendingApprovals application use-case handler.
 */

#pragma once

#include "haven/application/reservations/list_pending_approvals_query.hpp"
#include "haven/application/reservations/reservation_repository.hpp"

namespace haven::application::reservations {

/**
 * @brief Lists reservations awaiting approval within one organization.
 */
class ListPendingApprovalsHandler final {
public:
    /**
     * @brief Constructs the handler with its required repository port.
     *
     * @param reservation_repository Tenant-aware reservation repository.
     */
    explicit ListPendingApprovalsHandler(
        ReservationRepository& reservation_repository) noexcept;

    /**
     * @brief Executes the tenant-scoped approval queue query.
     *
     * @param query Organization used to scope the queue.
     * @return Reservations currently awaiting approval.
     */
    [[nodiscard]] ReservationListResult handle(
        const ListPendingApprovalsQuery& query) const;

private:
    ReservationRepository& reservation_repository_;
};

}  // namespace haven::application::reservations