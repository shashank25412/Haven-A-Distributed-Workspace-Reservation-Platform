/**
 * @file reject_reservation_result.hpp
 * @brief Defines outcomes produced by the RejectReservation use case.
 */

#pragma once

#include "haven/domain/reservation.hpp"

#include <optional>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Identifies the outcome of a reservation rejection attempt.
 */
enum class RejectReservationStatus {
    REJECTED,
    RESERVATION_NOT_FOUND,
    INVALID_STATE
};

/**
 * @brief Represents the result of rejecting a reservation.
 */
class RejectReservationResult final {
public:
    /**
     * @brief Creates a successful rejection result.
     *
     * @param reservation Rejected reservation aggregate.
     */
    [[nodiscard]] static RejectReservationResult rejected(
        haven::domain::Reservation reservation) {
        return RejectReservationResult{
            RejectReservationStatus::REJECTED,
            std::move(reservation)};
    }

    /**
     * @brief Creates an unsuccessful rejection result.
     *
     * @param status Rejection failure outcome.
     */
    [[nodiscard]] static RejectReservationResult failed(
        const RejectReservationStatus status) {
        return RejectReservationResult{status, std::nullopt};
    }

    /**
     * @brief Returns the rejection outcome.
     */
    [[nodiscard]] RejectReservationStatus status() const noexcept {
        return status_;
    }

    /**
     * @brief Returns the rejected reservation when successful.
     */
    [[nodiscard]] const std::optional<haven::domain::Reservation>& reservation() const noexcept {
        return reservation_;
    }

private:
    RejectReservationResult(
        const RejectReservationStatus status,
        std::optional<haven::domain::Reservation> reservation)
        : status_(status),
          reservation_(std::move(reservation)) {}

    RejectReservationStatus status_;
    std::optional<haven::domain::Reservation> reservation_;
};

}  // namespace haven::application::reservations