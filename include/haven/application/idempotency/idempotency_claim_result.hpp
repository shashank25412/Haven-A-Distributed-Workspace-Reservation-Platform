/**
 * @file idempotency_claim_result.hpp
 * @brief Defines outcomes from atomically claiming an idempotent operation.
 */

#pragma once

#include "haven/application/idempotency/idempotency_record.hpp"

#include <utility>

namespace haven::application::idempotency {

/** @brief Classifies every expected result of an atomic idempotency claim. */
enum class IdempotencyClaimStatus {
    Claimed,
    ExistingProcessing,
    ExistingSucceeded,
    ExistingFailedPermanent,
    FingerprintMismatch
};

/**
 * @brief Returns a classified claim outcome together with the stored record.
 *
 * Expected contention and fingerprint mismatch are values rather than
 * exceptions. Every outcome carries the authoritative stored record, allowing
 * callers to replay terminal results or inspect the generated operation IDs.
 */
class IdempotencyClaimResult final {
public:
    [[nodiscard]] static IdempotencyClaimResult claimed(IdempotencyRecord record) {
        return IdempotencyClaimResult{
            IdempotencyClaimStatus::Claimed, std::move(record), IdempotencyStatus::Processing};
    }

    [[nodiscard]] static IdempotencyClaimResult existing_processing(IdempotencyRecord record) {
        return IdempotencyClaimResult{IdempotencyClaimStatus::ExistingProcessing,
                                      std::move(record),
                                      IdempotencyStatus::Processing};
    }

    [[nodiscard]] static IdempotencyClaimResult existing_succeeded(IdempotencyRecord record) {
        return IdempotencyClaimResult{IdempotencyClaimStatus::ExistingSucceeded,
                                      std::move(record),
                                      IdempotencyStatus::Succeeded};
    }

    [[nodiscard]] static IdempotencyClaimResult existing_failed_permanently(
        IdempotencyRecord record) {
        return IdempotencyClaimResult{IdempotencyClaimStatus::ExistingFailedPermanent,
                                      std::move(record),
                                      IdempotencyStatus::FailedPermanent};
    }

    [[nodiscard]] static IdempotencyClaimResult fingerprint_mismatch(IdempotencyRecord record) {
        return IdempotencyClaimResult{IdempotencyClaimStatus::FingerprintMismatch,
                                      std::move(record)};
    }

    [[nodiscard]] IdempotencyClaimStatus status() const noexcept {
        return status_;
    }
    [[nodiscard]] const IdempotencyRecord& record() const noexcept {
        return record_;
    }

private:
    IdempotencyClaimResult(IdempotencyClaimStatus status,
                           IdempotencyRecord record,
                           IdempotencyStatus required_record_status)
        : status_(status), record_(std::move(record)) {
        if (record_.status() != required_record_status) {
            throw std::invalid_argument(
                "Idempotency claim result has an incompatible record state");
        }
    }

    IdempotencyClaimResult(IdempotencyClaimStatus status, IdempotencyRecord record)
        : status_(status), record_(std::move(record)) {}

    IdempotencyClaimStatus status_;
    IdempotencyRecord record_;
};

}  // namespace haven::application::idempotency
