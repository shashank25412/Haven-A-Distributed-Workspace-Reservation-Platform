/**
 * @file create_reservation_result_snapshot.cpp
 * @brief Implements reservation-create replay snapshot invariants.
 */

#include "haven/application/idempotency/create_reservation_result_snapshot.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::idempotency {
namespace {

using haven::application::reservations::CreateReservationStatus;

[[nodiscard]] bool is_successful(const CreateReservationStatus status) noexcept {
    return status == CreateReservationStatus::CREATED_CONFIRMED ||
           status == CreateReservationStatus::CREATED_PENDING_APPROVAL;
}

[[nodiscard]] bool is_permanent_rejection(const CreateReservationStatus status) noexcept {
    switch (status) {
        case CreateReservationStatus::RESOURCE_NOT_FOUND:
        case CreateReservationStatus::RESOURCE_INACTIVE:
        case CreateReservationStatus::SCHEDULE_CONFLICT:
        case CreateReservationStatus::POLICY_REJECTED:
            return true;
        case CreateReservationStatus::CREATED_CONFIRMED:
        case CreateReservationStatus::CREATED_PENDING_APPROVAL:
            return false;
    }
    return false;
}

}  // namespace

CreateReservationResultSnapshot CreateReservationResultSnapshot::successful(
    const CreateReservationStatus creation_status,
    haven::domain::ReservationId reservation_id,
    haven::domain::ResourceId resource_id,
    const haven::domain::ReservationStatus reservation_status,
    const haven::domain::ReservationKind reservation_kind,
    const haven::domain::Version initial_version,
    const TimePoint created_at) {
    if (!is_successful(creation_status)) {
        throw std::invalid_argument("Successful idempotency snapshot requires a creation status");
    }
    const bool status_matches =
        (creation_status == CreateReservationStatus::CREATED_CONFIRMED &&
         reservation_status == haven::domain::ReservationStatus::Confirmed) ||
        (creation_status == CreateReservationStatus::CREATED_PENDING_APPROVAL &&
         reservation_status == haven::domain::ReservationStatus::PendingApproval);
    if (!status_matches) {
        throw std::invalid_argument(
            "Creation and reservation statuses must describe the same result");
    }
    if (initial_version.value() == 0) {
        throw std::invalid_argument("Successful idempotency snapshot requires a positive version");
    }
    return CreateReservationResultSnapshot{creation_status,
                                           std::move(reservation_id),
                                           std::move(resource_id),
                                           reservation_status,
                                           reservation_kind,
                                           initial_version,
                                           created_at};
}

CreateReservationResultSnapshot CreateReservationResultSnapshot::permanent_rejection(
    const CreateReservationStatus creation_status) {
    if (!is_permanent_rejection(creation_status)) {
        throw std::invalid_argument("Permanent idempotency snapshot requires a rejection status");
    }
    return CreateReservationResultSnapshot{creation_status,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt};
}

CreateReservationResultSnapshot::CreateReservationResultSnapshot(
    const CreateReservationStatus creation_status,
    std::optional<haven::domain::ReservationId> reservation_id,
    std::optional<haven::domain::ResourceId> resource_id,
    std::optional<haven::domain::ReservationStatus> reservation_status,
    std::optional<haven::domain::ReservationKind> reservation_kind,
    std::optional<haven::domain::Version> initial_version,
    std::optional<TimePoint> created_at)
    : creation_status_(creation_status),
      reservation_id_(std::move(reservation_id)),
      resource_id_(std::move(resource_id)),
      reservation_status_(reservation_status),
      reservation_kind_(reservation_kind),
      initial_version_(initial_version),
      created_at_(created_at) {}

bool CreateReservationResultSnapshot::is_success() const noexcept {
    return reservation_id_.has_value();
}
CreateReservationStatus CreateReservationResultSnapshot::creation_status() const noexcept {
    return creation_status_;
}
const std::optional<haven::domain::ReservationId>& CreateReservationResultSnapshot::reservation_id()
    const noexcept {
    return reservation_id_;
}
const std::optional<haven::domain::ResourceId>& CreateReservationResultSnapshot::resource_id()
    const noexcept {
    return resource_id_;
}
const std::optional<haven::domain::ReservationStatus>&
CreateReservationResultSnapshot::reservation_status() const noexcept {
    return reservation_status_;
}
const std::optional<haven::domain::ReservationKind>&
CreateReservationResultSnapshot::reservation_kind() const noexcept {
    return reservation_kind_;
}
const std::optional<haven::domain::Version>& CreateReservationResultSnapshot::initial_version()
    const noexcept {
    return initial_version_;
}
const std::optional<CreateReservationResultSnapshot::TimePoint>&
CreateReservationResultSnapshot::created_at() const noexcept {
    return created_at_;
}

}  // namespace haven::application::idempotency
