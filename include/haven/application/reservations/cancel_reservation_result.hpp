/**
 * @file cancel_reservation_result.hpp
 * @brief Defines outcomes produced by the CancelReservation use case.
 */

#pragma once

#include "haven/domain/reservation.hpp"

#include <optional>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Identifies the outcome of a reservation cancellation attempt.
 */
enum class CancelReservationStatus {
    CANCELLED,
    RESERVATION_NOT_FOUND,
    CALLER_NOT_AUTHORIZED,
    INVALID_STATE
};

/**
 * @brief Represents the result of cancelling a reservation.
 */
class CancelReservationResult final {
public:
    /**
     * @brief Creates a successful cancellation result.
     *
     * @param reservation Cancelled reservation aggregate.
     */
    [[nodiscard]] static CancelReservationResult cancelled(
        haven::domain::Reservation reservation) {
        return CancelReservationResult{
            CancelReservationStatus::CANCELLED,
            std::move(reservation)};
    }

    /**
     * @brief Creates a rejected cancellation result.
     *
     * @param status Rejection outcome.
     */
    [[nodiscard]] static CancelReservationResult rejected(
        const CancelReservationStatus status) {
        return CancelReservationResult{status, std::nullopt};
    }

    /**
     * @brief Returns the cancellation outcome.
     */
    [[nodiscard]] CancelReservationStatus status() const noexcept {
        return status_;
    }

    /**
     * @brief Returns the cancelled reservation when successful.
     */
    [[nodiscard]] const std::optional<haven::domain::Reservation>& reservation() const noexcept {
        return reservation_;
    }

private:
    CancelReservationResult(
        const CancelReservationStatus status,
        std::optional<haven::domain::Reservation> reservation)
        : status_(status),
          reservation_(std::move(reservation)) {}

    CancelReservationStatus status_;
    std::optional<haven::domain::Reservation> reservation_;
};

}  // namespace haven::application::reservations