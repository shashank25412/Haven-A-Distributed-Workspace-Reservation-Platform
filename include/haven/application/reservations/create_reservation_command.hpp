/**
 * @file create_reservation_command.hpp
 * @brief Defines the input command for creating a reservation.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/idempotency_key.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"

#include <chrono>
#include <utility>

namespace haven::application::reservations {

/**
 * @brief Describes a request to create a reservation.
 *
 * Identifier generation and clock access remain outside the handler. This
 * keeps the application use case deterministic and independently testable.
 */
class CreateReservationCommand final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    /**
     * @brief Constructs a reservation creation command.
     *
     * @param organization_id Organization visible to the caller.
     * @param reservation_id Externally generated reservation identifier.
     * @param resource_id Resource requested by the caller.
     * @param creator_id User creating the reservation.
     * @param interval Requested reservation interval.
     * @param purpose Caller-supplied purpose text.
     * @param reservation_kind Standard or maintenance reservation kind.
     * @param maintenance_authorized Whether maintenance authorization was explicitly granted.
     * @param created_event_id Identifier for the reservation-created domain event.
     * @param confirmed_event_id Identifier for a possible reservation-confirmed event.
     * @param approval_requested_event_id Identifier for a possible approval-requested event.
     * @param occurred_at Timestamp applied to generated domain events.
     */
    CreateReservationCommand(haven::domain::OrganizationId organization_id,
                             haven::domain::IdempotencyKey idempotency_key,
                             haven::domain::ReservationId reservation_id,
                             haven::domain::ResourceId resource_id,
                             haven::domain::UserId creator_id,
                             haven::domain::TimeInterval interval,
                             haven::domain::Purpose purpose,
                             haven::domain::ReservationKind reservation_kind,
                             bool maintenance_authorized,
                             haven::domain::EventId created_event_id,
                             haven::domain::EventId confirmed_event_id,
                             haven::domain::EventId approval_requested_event_id,
                             TimePoint occurred_at)
        : organization_id_(std::move(organization_id)),
          idempotency_key_(std::move(idempotency_key)),
          reservation_id_(std::move(reservation_id)),
          resource_id_(std::move(resource_id)),
          creator_id_(std::move(creator_id)),
          interval_(std::move(interval)),
          purpose_(std::move(purpose)),
          reservation_kind_(reservation_kind),
          maintenance_authorized_(maintenance_authorized),
          created_event_id_(std::move(created_event_id)),
          confirmed_event_id_(std::move(confirmed_event_id)),
          approval_requested_event_id_(std::move(approval_requested_event_id)),
          occurred_at_(occurred_at) {}

    /**
     * @brief Returns the organization used to scope the operation.
     */
    [[nodiscard]] const haven::domain::OrganizationId& organization_id() const noexcept {
        return organization_id_;
    }

    [[nodiscard]] const haven::domain::IdempotencyKey& idempotency_key() const noexcept {
        return idempotency_key_;
    }

    /**
     * @brief Returns the reservation identifier.
     */
    [[nodiscard]] const haven::domain::ReservationId& reservation_id() const noexcept {
        return reservation_id_;
    }

    /**
     * @brief Returns the requested resource identifier.
     */
    [[nodiscard]] const haven::domain::ResourceId& resource_id() const noexcept {
        return resource_id_;
    }

    /**
     * @brief Returns the user creating the reservation.
     */
    [[nodiscard]] const haven::domain::UserId& creator_id() const noexcept {
        return creator_id_;
    }

    /**
     * @brief Returns the requested reservation interval.
     */
    [[nodiscard]] const haven::domain::TimeInterval& interval() const noexcept {
        return interval_;
    }

    /**
     * @brief Returns the caller-supplied reservation purpose.
     */
    [[nodiscard]] const haven::domain::Purpose& purpose() const noexcept {
        return purpose_;
    }

    /**
     * @brief Returns the requested reservation kind.
     */
    [[nodiscard]] haven::domain::ReservationKind reservation_kind() const noexcept {
        return reservation_kind_;
    }

    /**
     * @brief Returns whether maintenance authorization was explicitly granted.
     */
    [[nodiscard]] bool maintenance_authorized() const noexcept {
        return maintenance_authorized_;
    }

    /**
     * @brief Returns the reservation-created event identifier.
     */
    [[nodiscard]] const haven::domain::EventId& created_event_id() const noexcept {
        return created_event_id_;
    }

    [[nodiscard]] const haven::domain::EventId& confirmed_event_id() const noexcept {
        return confirmed_event_id_;
    }

    /**
     * @brief Returns the approval-requested event identifier.
     */
    [[nodiscard]] const haven::domain::EventId& approval_requested_event_id() const noexcept {
        return approval_requested_event_id_;
    }

    /**
     * @brief Returns the timestamp applied to generated domain events.
     */
    [[nodiscard]] TimePoint occurred_at() const noexcept {
        return occurred_at_;
    }

private:
    haven::domain::OrganizationId organization_id_;
    haven::domain::IdempotencyKey idempotency_key_;
    haven::domain::ReservationId reservation_id_;
    haven::domain::ResourceId resource_id_;
    haven::domain::UserId creator_id_;
    haven::domain::TimeInterval interval_;
    haven::domain::Purpose purpose_;
    haven::domain::ReservationKind reservation_kind_;
    bool maintenance_authorized_;
    haven::domain::EventId created_event_id_;
    haven::domain::EventId confirmed_event_id_;
    haven::domain::EventId approval_requested_event_id_;
    TimePoint occurred_at_;
};

}  // namespace haven::application::reservations
