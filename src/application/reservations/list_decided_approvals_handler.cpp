/**
 * @file list_decided_approvals_handler.cpp
 * @brief Implements the ListDecidedApprovals application use case.
 */

#include "haven/application/reservations/list_decided_approvals_handler.hpp"

#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

#include <algorithm>

namespace haven::application::reservations {

namespace {

[[nodiscard]] bool is_decided(const haven::domain::Reservation& reservation) {
    if (reservation.status() == haven::domain::ReservationStatus::Rejected) {
        return true;
    }
    return reservation.status() == haven::domain::ReservationStatus::Confirmed &&
          reservation.approval_info().has_value();
}

}  // namespace

ListDecidedApprovalsHandler::ListDecidedApprovalsHandler(
    ReservationRepository& reservation_repository) noexcept
    : reservation_repository_(reservation_repository) {}

ReservationListResult ListDecidedApprovalsHandler::handle(
    const ListDecidedApprovalsQuery& query) const {
    HVN_TRACE_SCOPE();

    auto reservations = reservation_repository_.find_decided_approvals(query.organization_id());

    const auto first_hidden_reservation = std::remove_if(
        reservations.begin(),
        reservations.end(),
        [&query](const haven::domain::Reservation& reservation) {
            return reservation.organization_id() != query.organization_id() ||
                  !is_decided(reservation);
        });

    if (first_hidden_reservation != reservations.end()) {
        HVN_WARN_LOG(
            "Reservation repository returned entries outside the requested decision history scope");
        reservations.erase(first_hidden_reservation, reservations.end());
    }

    return reservations;
}

}  // namespace haven::application::reservations
