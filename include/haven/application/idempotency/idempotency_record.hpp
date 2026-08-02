/**
 * @file idempotency_record.hpp
 * @brief Defines the application state of one idempotent create operation.
 */

#pragma once

#include "haven/application/idempotency/create_reservation_result_snapshot.hpp"
#include "haven/application/idempotency/idempotency_fingerprint.hpp"
#include "haven/application/idempotency/idempotency_scope.hpp"
#include "haven/application/idempotency/idempotency_status.hpp"
#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"

#include <chrono>
#include <optional>

namespace haven::application::idempotency {

/** @brief Server-generated identities fixed when an operation first enters processing. */
struct ReservationCreationIdentifiers final {
    haven::domain::ReservationId reservation_id;
    haven::domain::EventId created_event_id;
    haven::domain::EventId confirmed_event_id;
    haven::domain::EventId approval_requested_event_id;

    bool operator==(const ReservationCreationIdentifiers&) const = default;
};

/**
 * @brief Models one scoped idempotent reservation-create operation.
 *
 * Scope, fingerprint, generated identifiers, and creation time are immutable
 * across terminal transitions. Couchbase CAS, PersistenceToken, and expiry are
 * intentionally absent from this application model.
 */
class IdempotencyRecord final {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    [[nodiscard]] static IdempotencyRecord processing(
        IdempotencyScope scope,
        IdempotencyFingerprint fingerprint,
        ReservationCreationIdentifiers generated_identifiers,
        TimePoint created_at);

    [[nodiscard]] static IdempotencyRecord succeeded(
        IdempotencyScope scope,
        IdempotencyFingerprint fingerprint,
        ReservationCreationIdentifiers generated_identifiers,
        TimePoint created_at,
        CreateReservationResultSnapshot snapshot);

    [[nodiscard]] static IdempotencyRecord failed_permanently(
        IdempotencyScope scope,
        IdempotencyFingerprint fingerprint,
        ReservationCreationIdentifiers generated_identifiers,
        TimePoint created_at,
        CreateReservationResultSnapshot snapshot);

    void record_succeeded(CreateReservationResultSnapshot snapshot);
    void record_failed_permanently(CreateReservationResultSnapshot snapshot);

    [[nodiscard]] const IdempotencyScope& scope() const noexcept;
    [[nodiscard]] const IdempotencyFingerprint& fingerprint() const noexcept;
    [[nodiscard]] IdempotencyStatus status() const noexcept;
    [[nodiscard]] const ReservationCreationIdentifiers& generated_identifiers() const noexcept;
    [[nodiscard]] TimePoint created_at() const noexcept;
    [[nodiscard]] const std::optional<CreateReservationResultSnapshot>& result() const noexcept;

private:
    IdempotencyRecord(IdempotencyScope scope,
                      IdempotencyFingerprint fingerprint,
                      ReservationCreationIdentifiers generated_identifiers,
                      TimePoint created_at,
                      IdempotencyStatus status,
                      std::optional<CreateReservationResultSnapshot> result);

    IdempotencyScope scope_;
    IdempotencyFingerprint fingerprint_;
    ReservationCreationIdentifiers generated_identifiers_;
    TimePoint created_at_;
    IdempotencyStatus status_;
    std::optional<CreateReservationResultSnapshot> result_;
};

}  // namespace haven::application::idempotency
