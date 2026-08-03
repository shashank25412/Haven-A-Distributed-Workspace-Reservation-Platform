/**
 * @file create_reservation_handler.hpp
 * @brief Declares the CreateReservation application use-case handler.
 */

#pragma once

#include "haven/application/idempotency/idempotency_repository.hpp"
#include "haven/application/observability/metrics/metrics_recorder.hpp"
#include "haven/application/reservations/create_reservation_command.hpp"
#include "haven/application/reservations/create_reservation_result.hpp"
#include "haven/application/reservations/metrics/reservation_creation_metrics.hpp"
#include "haven/application/reservations/reservation_creation_event_store.hpp"
#include "haven/application/reservations/reservation_creation_store.hpp"
#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/domain/policies/reservation_creation_policy.hpp"

#include <chrono>

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
     * @param metrics_recorder Best-effort reservation workflow metrics sink.
     */
    CreateReservationHandler(
        haven::application::resources::ResourceRepository& resource_repository,
        ReservationRepository& reservation_repository,
        ReservationCreationStore& reservation_creation_store,
        ReservationCreationEventStore& reservation_creation_event_store,
        haven::application::idempotency::IdempotencyRepository& idempotency_repository,
        const haven::domain::ReservationCreationPolicy& reservation_creation_policy,
        haven::application::observability::metrics::MetricsRecorder& metrics_recorder) noexcept;

    /**
     * @brief Executes reservation creation.
     *
     * @param command Reservation creation input.
     * @return Successful or rejected application result.
     */
    [[nodiscard]] CreateReservationResult handle(const CreateReservationCommand& command) const;

private:
    [[nodiscard]] CreateReservationResult handle_core(
        const CreateReservationCommand& command,
        metrics::ReservationCreationOutcome& outcome) const;
    void record_metrics(metrics::ReservationCreationOutcome outcome,
                        std::chrono::steady_clock::time_point started_at) const noexcept;

    haven::application::resources::ResourceRepository& resource_repository_;
    ReservationRepository& reservation_repository_;
    ReservationCreationStore& reservation_creation_store_;
    ReservationCreationEventStore& reservation_creation_event_store_;
    haven::application::idempotency::IdempotencyRepository& idempotency_repository_;
    const haven::domain::ReservationCreationPolicy& reservation_creation_policy_;
    haven::application::observability::metrics::MetricsRecorder& metrics_recorder_;
};

}  // namespace haven::application::reservations
