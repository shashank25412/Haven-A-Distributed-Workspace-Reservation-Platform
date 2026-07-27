/**
 * @file list_pending_approvals_handler.cpp
 * @brief Implements the ListPendingApprovals application use case.
 */

#include "haven/application/reservations/list_pending_approvals_handler.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::reservations {

ListPendingApprovalsHandler::ListPendingApprovalsHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

ReservationListResult ListPendingApprovalsHandler::handle(
    const ListPendingApprovalsQuery& query) const {
    HVN_TRACE_SCOPE();

    auto reservations = reservation_repository_.find_pending_approvals(
        query.organization_id());

    const auto first_hidden_reservation = std::remove_if(
        reservations.begin(),
        reservations.end(),
        [&query](const haven::domain::Reservation& reservation) {
            return reservation.organization_id() != query.organization_id()
                || reservation.status()
                    != haven::domain::ReservationStatus::PendingApproval;
        });

    if (first_hidden_reservation != reservations.end()) {
        HVN_WARN_LOG(
            "Reservation repository returned entries outside the requested approval scope");
        reservations.erase(first_hidden_reservation, reservations.end());
    }

    return reservations;
}

}  // namespace haven::application::reservations
