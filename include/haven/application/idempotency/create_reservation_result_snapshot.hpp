/**
 * @file create_reservation_result_snapshot.hpp
 * @brief Defines a presentation-neutral reservation-create replay snapshot.
 */

#pragma once

#include "haven/application/reservations/create_reservation_result.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/purpose.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/domain/value_objects/resource_id.hpp"
#include "haven/domain/value_objects/time_interval.hpp"
#include "haven/domain/value_objects/user_id.hpp"
#include "haven/domain/value_objects/version.hpp"

#include <chrono>
#include <optional>

namespace haven::application::idempotency {

/**
 * @brief Captures the immutable application outcome of an original create attempt.
 *
 * This snapshot deliberately excludes HTTP responses and persistence revisions.
 * Transient infrastructure failures are not eligible for permanent snapshots.
 */
class CreateReservationResultSnapshot final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    [[nodiscard]] static CreateReservationResultSnapshot successful(
        haven::application::reservations::CreateReservationStatus creation_status,
        haven::domain::OrganizationId organization_id,
        haven::domain::ReservationId reservation_id,
        haven::domain::ResourceId resource_id,
        haven::domain::UserId creator_id,
        haven::domain::TimeInterval interval,
        haven::domain::Purpose purpose,
        haven::domain::ReservationStatus reservation_status,
        haven::domain::ReservationKind reservation_kind,
        haven::domain::Version initial_version,
        TimePoint created_at);

    /** @brief Compatibility factory for callers that do not reconstruct replay aggregates. */
    [[nodiscard]] static CreateReservationResultSnapshot successful(
        haven::application::reservations::CreateReservationStatus creation_status,
        haven::domain::ReservationId reservation_id,
        haven::domain::ResourceId resource_id,
        haven::domain::ReservationStatus reservation_status,
        haven::domain::ReservationKind reservation_kind,
        haven::domain::Version initial_version,
        TimePoint created_at);

    [[nodiscard]] static CreateReservationResultSnapshot permanent_rejection(
        haven::application::reservations::CreateReservationStatus creation_status);

    [[nodiscard]] bool is_success() const noexcept;
    [[nodiscard]] haven::application::reservations::CreateReservationStatus creation_status()
        const noexcept;
    [[nodiscard]] const std::optional<haven::domain::OrganizationId>& organization_id()
        const noexcept;
    [[nodiscard]] const std::optional<haven::domain::ReservationId>& reservation_id()
        const noexcept;
    [[nodiscard]] const std::optional<haven::domain::ResourceId>& resource_id() const noexcept;
    [[nodiscard]] const std::optional<haven::domain::UserId>& creator_id() const noexcept;
    [[nodiscard]] const std::optional<haven::domain::TimeInterval>& interval() const noexcept;
    [[nodiscard]] const std::optional<haven::domain::Purpose>& purpose() const noexcept;
    [[nodiscard]] const std::optional<haven::domain::ReservationStatus>& reservation_status()
        const noexcept;
    [[nodiscard]] const std::optional<haven::domain::ReservationKind>& reservation_kind()
        const noexcept;
    [[nodiscard]] const std::optional<haven::domain::Version>& initial_version() const noexcept;
    [[nodiscard]] const std::optional<TimePoint>& created_at() const noexcept;

    bool operator==(const CreateReservationResultSnapshot&) const = default;

private:
    CreateReservationResultSnapshot(
        haven::application::reservations::CreateReservationStatus creation_status,
        std::optional<haven::domain::OrganizationId> organization_id,
        std::optional<haven::domain::ReservationId> reservation_id,
        std::optional<haven::domain::ResourceId> resource_id,
        std::optional<haven::domain::UserId> creator_id,
        std::optional<haven::domain::TimeInterval> interval,
        std::optional<haven::domain::Purpose> purpose,
        std::optional<haven::domain::ReservationStatus> reservation_status,
        std::optional<haven::domain::ReservationKind> reservation_kind,
        std::optional<haven::domain::Version> initial_version,
        std::optional<TimePoint> created_at);

    haven::application::reservations::CreateReservationStatus creation_status_;
    std::optional<haven::domain::OrganizationId> organization_id_;
    std::optional<haven::domain::ReservationId> reservation_id_;
    std::optional<haven::domain::ResourceId> resource_id_;
    std::optional<haven::domain::UserId> creator_id_;
    std::optional<haven::domain::TimeInterval> interval_;
    std::optional<haven::domain::Purpose> purpose_;
    std::optional<haven::domain::ReservationStatus> reservation_status_;
    std::optional<haven::domain::ReservationKind> reservation_kind_;
    std::optional<haven::domain::Version> initial_version_;
    std::optional<TimePoint> created_at_;
};

}  // namespace haven::application::idempotency
