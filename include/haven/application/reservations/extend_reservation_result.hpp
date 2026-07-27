/**
 * @file extend_reservation_result.hpp
 * @brief Defines outcomes produced by the ExtendReservation use case.
 */

#pragma once

#include "haven/domain/reservation.hpp"

#include <optional>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Identifies the outcome of a reservation extension attempt.
 */
enum class ExtendReservationStatus {
    EXTENDED,
    RESERVATION_NOT_FOUND,
    CALLER_NOT_AUTHORIZED,
    INVALID_STATE,
    SCHEDULE_CONFLICT,
    POLICY_REJECTED
};

/**
 * @brief Represents the result of extending a reservation.
 */
class ExtendReservationResult final {
public:
    /**
     * @brief Creates a successful extension result.
     *
     * @param reservation Updated reservation aggregate.
     */
    [[nodiscard]] static ExtendReservationResult extended(
        haven::domain::Reservation reservation) {
        return ExtendReservationResult{
            ExtendReservationStatus::EXTENDED,
            std::move(reservation)};
    }

    /**
     * @brief Creates a rejected extension result.
     *
     * @param status Rejection outcome.
     */
    [[nodiscard]] static ExtendReservationResult rejected(
        const ExtendReservationStatus status) {
        return ExtendReservationResult{status, std::nullopt};
    }

    /**
     * @brief Returns the extension outcome.
     */
    [[nodiscard]] ExtendReservationStatus status() const noexcept {
        return status_;
    }

    /**
     * @brief Returns the updated reservation when successful.
     */
    [[nodiscard]] const std::optional<haven::domain::Reservation>& reservation() const noexcept {
        return reservation_;
    }

private:
    ExtendReservationResult(
        const ExtendReservationStatus status,
        std::optional<haven::domain::Reservation> reservation)
        : status_(status),
          reservation_(std::move(reservation)) {}

    ExtendReservationStatus status_;
    std::optional<haven::domain::Reservation> reservation_;
};

}  // namespace haven::application::reservations