/**
 * @file create_reservation_handler.hpp
 * @brief Declares the CreateReservation application use-case handler.
 */

#pragma once

#include "haven/application/reservations/create_reservation_command.hpp"
#include "haven/application/reservations/create_reservation_result.hpp"
#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/policies/reservation_creation_policy.hpp"

namespace haven::application::reservations {

/**
 * @brief Coordinates tenant-safe reservation creation.
 *
 * The handler loads the resource, evaluates domain policies, checks schedule
 * conflicts, creates the appropriate aggregate state, and persists the result.
 */
class CreateReservationHandler final {
public:
    /**
     * @brief Constructs the handler with its required ports and domain policy.
     *
     * @param resource_repository Tenant-aware resource repository.
     * @param reservation_repository Tenant-aware reservation repository.
     * @param reservation_creation_policy Domain policy governing reservation creation.
     */
    CreateReservationHandler(
        haven::application::resources::ResourceRepository& resource_repository,
        ReservationRepository& reservation_repository,
        const haven::domain::ReservationCreationPolicy& reservation_creation_policy) noexcept;

    /**
     * @brief Executes reservation creation.
     *
     * @param command Reservation creation input.
     * @return Successful or rejected application result.
     */
    [[nodiscard]] CreateReservationResult handle(
        const CreateReservationCommand& command) const;

private:
    haven::application::resources::ResourceRepository& resource_repository_;
    ReservationRepository& reservation_repository_;
    const haven::domain::ReservationCreationPolicy& reservation_creation_policy_;
};

}  // namespace haven::application::reservations
