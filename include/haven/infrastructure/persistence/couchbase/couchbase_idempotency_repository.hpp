/**
 * @file couchbase_idempotency_repository.hpp
 * @brief Declares the Couchbase idempotency repository implementation.
 */

#pragma once

#include "haven/application/idempotency/idempotency_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include <chrono>
#include <memory>

namespace haven::infrastructure::persistence::couchbase {

class CouchbaseIdempotencyRepository final
    : public haven::application::idempotency::IdempotencyRepository {
public:
    CouchbaseIdempotencyRepository(std::shared_ptr<CouchbaseConnection> connection,
                                   std::chrono::seconds retention);

    [[nodiscard]] haven::application::idempotency::IdempotencyClaimResult claim(
        const haven::application::idempotency::IdempotencyRecord& processing_record) override;
    [[nodiscard]] std::optional<haven::application::idempotency::IdempotencyRecord> find(
        const haven::application::idempotency::IdempotencyScope& scope) const override;
    void record_succeeded(
        const haven::application::idempotency::IdempotencyScope& scope,
        const haven::application::idempotency::IdempotencyFingerprint& expected_fingerprint,
        const haven::application::idempotency::CreateReservationResultSnapshot& snapshot) override;
    void record_failed_permanently(
        const haven::application::idempotency::IdempotencyScope& scope,
        const haven::application::idempotency::IdempotencyFingerprint& expected_fingerprint,
        const haven::application::idempotency::CreateReservationResultSnapshot& snapshot) override;

private:
    void complete(const haven::application::idempotency::IdempotencyScope& scope,
                  const haven::application::idempotency::IdempotencyFingerprint& fingerprint,
                  const haven::application::idempotency::CreateReservationResultSnapshot& snapshot,
                  bool succeeded);

    std::shared_ptr<CouchbaseConnection> connection_;
    std::chrono::seconds retention_;
};

}  // namespace haven::infrastructure::persistence::couchbase
