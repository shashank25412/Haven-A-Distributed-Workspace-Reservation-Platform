/**
 * @file create_reservation_result.hpp
 * @brief Defines outcomes produced by the CreateReservation use case.
 */

#pragma once

#include "haven/domain/reservation.hpp"

#include <optional>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Identifies the outcome of a reservation creation attempt.
 */
enum class CreateReservationStatus {
    CREATED_CONFIRMED,
    CREATED_PENDING_APPROVAL,
    RESOURCE_NOT_FOUND,
    RESOURCE_INACTIVE,
    SCHEDULE_CONFLICT,
    POLICY_REJECTED
};

/**
 * @brief Represents the result of creating a reservation.
 *
 * Successful results contain the created reservation. Rejected results do not
 * reveal resource information beyond the application-owned status.
 */
class CreateReservationResult final {
public:
    /**
     * @brief Creates a successful result containing a confirmed reservation.
     */
    [[nodiscard]] static CreateReservationResult confirmed(
        haven::domain::Reservation reservation) {
        return CreateReservationResult{
            CreateReservationStatus::CREATED_CONFIRMED,
            std::move(reservation)};
    }

    /**
     * @brief Creates a successful result containing a pending reservation.
     */
    [[nodiscard]] static CreateReservationResult pending_approval(
        haven::domain::Reservation reservation) {
        return CreateReservationResult{
            CreateReservationStatus::CREATED_PENDING_APPROVAL,
            std::move(reservation)};
    }

    /**
     * @brief Creates a rejected result.
     *
     * @param status Rejection status.
     */
    [[nodiscard]] static CreateReservationResult rejected(
        const CreateReservationStatus status) {
        return CreateReservationResult{status, std::nullopt};
    }

    /**
     * @brief Returns the application outcome.
     */
    [[nodiscard]] CreateReservationStatus status() const noexcept {
        return status_;
    }

    /**
     * @brief Returns the created reservation when the operation succeeded.
     */
    [[nodiscard]] const std::optional<haven::domain::Reservation>& reservation() const noexcept {
        return reservation_;
    }

private:
    CreateReservationResult(
        const CreateReservationStatus status,
        std::optional<haven::domain::Reservation> reservation)
        : status_(status),
          reservation_(std::move(reservation)) {}

    CreateReservationStatus status_;
    std::optional<haven::domain::Reservation> reservation_;
};

}  // namespace haven::application::reservations