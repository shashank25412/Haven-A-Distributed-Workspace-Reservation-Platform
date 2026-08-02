/**
 * @file idempotency_repository.hpp
 * @brief Defines application-owned idempotency persistence operations.
 */

#pragma once

#include "haven/application/idempotency/create_reservation_result_snapshot.hpp"
#include "haven/application/idempotency/idempotency_claim_result.hpp"

#include <optional>

namespace haven::application::idempotency {

/**
 * @brief Persists and resolves scoped idempotent operations.
 *
 * Claim is atomic from the caller's perspective. Expected existing states are
 * returned as values. Completion hides datastore CAS and permits an equivalent
 * repeated terminal write as an idempotent no-op; conflicting terminal writes
 * are rejected and terminal records remain immutable.
 */
class IdempotencyRepository {
public:
    virtual ~IdempotencyRepository() = default;

    [[nodiscard]] virtual IdempotencyClaimResult claim(
        const IdempotencyRecord& processing_record) = 0;

    [[nodiscard]] virtual std::optional<IdempotencyRecord> find(
        const IdempotencyScope& scope) const = 0;

    virtual void record_succeeded(const IdempotencyScope& scope,
                                  const IdempotencyFingerprint& expected_fingerprint,
                                  const CreateReservationResultSnapshot& snapshot) = 0;

    virtual void record_failed_permanently(const IdempotencyScope& scope,
                                           const IdempotencyFingerprint& expected_fingerprint,
                                           const CreateReservationResultSnapshot& snapshot) = 0;
};

}  // namespace haven::application::idempotency
