/**
 * @file approve_reservation_result.hpp
 * @brief Defines outcomes produced by the ApproveReservation use case.
 */

#pragma once

#include "haven/domain/reservation.hpp"

#include <optional>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Identifies the outcome of a reservation approval attempt.
 */
enum class ApproveReservationStatus {
    APPROVED,
    RESERVATION_NOT_FOUND,
    INVALID_STATE,
    SCHEDULE_CONFLICT
};

/**
 * @brief Represents the result of approving a reservation.
 */
class ApproveReservationResult final {
public:
    /**
     * @brief Creates a successful approval result.
     *
     * @param reservation Approved reservation aggregate.
     */
    [[nodiscard]] static ApproveReservationResult approved(
        haven::domain::Reservation reservation) {
        return ApproveReservationResult{
            ApproveReservationStatus::APPROVED,
            std::move(reservation)};
    }

    /**
     * @brief Creates a rejected approval result.
     *
     * @param status Rejection outcome.
     */
    [[nodiscard]] static ApproveReservationResult rejected(
        const ApproveReservationStatus status) {
        return ApproveReservationResult{status, std::nullopt};
    }

    /**
     * @brief Returns the approval outcome.
     */
    [[nodiscard]] ApproveReservationStatus status() const noexcept {
        return status_;
    }

    /**
     * @brief Returns the approved reservation when successful.
     */
    [[nodiscard]] const std::optional<haven::domain::Reservation>& reservation() const noexcept {
        return reservation_;
    }

private:
    ApproveReservationResult(
        const ApproveReservationStatus status,
        std::optional<haven::domain::Reservation> reservation)
        : status_(status),
          reservation_(std::move(reservation)) {}

    ApproveReservationStatus status_;
    std::optional<haven::domain::Reservation> reservation_;
};

}  // namespace haven::application::reservations