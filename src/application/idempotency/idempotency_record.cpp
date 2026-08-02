/**
 * @file idempotency_record.cpp
 * @brief Implements idempotent create-operation state invariants.
 */

#include "haven/application/idempotency/idempotency_record.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::idempotency {
namespace {

void validate_success_snapshot(const CreateReservationResultSnapshot& snapshot,
                               const ReservationCreationIdentifiers& generated_identifiers,
                               const IdempotencyRecord::TimePoint created_at) {
    if (!snapshot.is_success()) {
        throw std::invalid_argument("Succeeded idempotency record requires a successful snapshot");
    }
    if (*snapshot.reservation_id() != generated_identifiers.reservation_id) {
        throw std::invalid_argument(
            "Successful snapshot must use the generated reservation identifier");
    }
    if (*snapshot.created_at() != created_at) {
        throw std::invalid_argument(
            "Successful snapshot must preserve the operation creation time");
    }
}

}  // namespace

IdempotencyRecord IdempotencyRecord::processing(
    IdempotencyScope scope,
    IdempotencyFingerprint fingerprint,
    ReservationCreationIdentifiers generated_identifiers,
    const TimePoint created_at) {
    return IdempotencyRecord{std::move(scope),
                             std::move(fingerprint),
                             std::move(generated_identifiers),
                             created_at,
                             IdempotencyStatus::Processing,
                             std::nullopt};
}

IdempotencyRecord IdempotencyRecord::succeeded(IdempotencyScope scope,
                                               IdempotencyFingerprint fingerprint,
                                               ReservationCreationIdentifiers generated_identifiers,
                                               const TimePoint created_at,
                                               CreateReservationResultSnapshot snapshot) {
    validate_success_snapshot(snapshot, generated_identifiers, created_at);
    return IdempotencyRecord{std::move(scope),
                             std::move(fingerprint),
                             std::move(generated_identifiers),
                             created_at,
                             IdempotencyStatus::Succeeded,
                             std::move(snapshot)};
}

IdempotencyRecord IdempotencyRecord::failed_permanently(
    IdempotencyScope scope,
    IdempotencyFingerprint fingerprint,
    ReservationCreationIdentifiers generated_identifiers,
    const TimePoint created_at,
    CreateReservationResultSnapshot snapshot) {
    if (snapshot.is_success()) {
        throw std::invalid_argument(
            "Permanently failed idempotency record requires a rejection snapshot");
    }
    return IdempotencyRecord{std::move(scope),
                             std::move(fingerprint),
                             std::move(generated_identifiers),
                             created_at,
                             IdempotencyStatus::FailedPermanent,
                             std::move(snapshot)};
}

void IdempotencyRecord::record_succeeded(CreateReservationResultSnapshot snapshot) {
    if (status_ != IdempotencyStatus::Processing) {
        throw std::logic_error("Only a processing idempotency record can succeed");
    }
    validate_success_snapshot(snapshot, generated_identifiers_, created_at_);
    status_ = IdempotencyStatus::Succeeded;
    result_ = std::move(snapshot);
}

void IdempotencyRecord::record_failed_permanently(CreateReservationResultSnapshot snapshot) {
    if (status_ != IdempotencyStatus::Processing) {
        throw std::logic_error("Only a processing idempotency record can fail permanently");
    }
    if (snapshot.is_success()) {
        throw std::invalid_argument(
            "Permanently failed idempotency record requires a rejection snapshot");
    }
    status_ = IdempotencyStatus::FailedPermanent;
    result_ = std::move(snapshot);
}

IdempotencyRecord::IdempotencyRecord(IdempotencyScope scope,
                                     IdempotencyFingerprint fingerprint,
                                     ReservationCreationIdentifiers generated_identifiers,
                                     const TimePoint created_at,
                                     const IdempotencyStatus status,
                                     std::optional<CreateReservationResultSnapshot> result)
    : scope_(std::move(scope)),
      fingerprint_(std::move(fingerprint)),
      generated_identifiers_(std::move(generated_identifiers)),
      created_at_(created_at),
      status_(status),
      result_(std::move(result)) {}

const IdempotencyScope& IdempotencyRecord::scope() const noexcept {
    return scope_;
}
const IdempotencyFingerprint& IdempotencyRecord::fingerprint() const noexcept {
    return fingerprint_;
}
IdempotencyStatus IdempotencyRecord::status() const noexcept {
    return status_;
}
const ReservationCreationIdentifiers& IdempotencyRecord::generated_identifiers() const noexcept {
    return generated_identifiers_;
}
IdempotencyRecord::TimePoint IdempotencyRecord::created_at() const noexcept {
    return created_at_;
}
const std::optional<CreateReservationResultSnapshot>& IdempotencyRecord::result() const noexcept {
    return result_;
}

}  // namespace haven::application::idempotency
