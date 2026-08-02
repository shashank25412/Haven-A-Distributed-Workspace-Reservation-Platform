/**
 * @file test_idempotency_repository.hpp
 * @brief Defines a deterministic in-memory idempotency repository for application tests.
 */

#pragma once

#include "haven/application/idempotency/idempotency_repository.hpp"
#include "haven/application/idempotency/idempotency_repository_error.hpp"
#include "haven/application/repository_error.hpp"

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace haven::test::application::idempotency {

/**
 * @brief Implements atomic claim and immutable terminal completion under one mutex.
 *
 * Copies are returned from storage so tests cannot mutate repository state by
 * retaining a reference. The forced failure hook models an unexpected generic
 * persistence failure without introducing infrastructure-specific errors.
 */
class TestIdempotencyRepository final
    : public haven::application::idempotency::IdempotencyRepository {
public:
    [[nodiscard]] haven::application::idempotency::IdempotencyClaimResult claim(
        const haven::application::idempotency::IdempotencyRecord& processing_record) override {
        using namespace haven::application::idempotency;
        if (processing_record.status() != IdempotencyStatus::Processing ||
            processing_record.result().has_value()) {
            throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::InvalidRecord,
                                             "Claim requires a Processing record"};
        }

        const auto lock = std::scoped_lock{mutex_};
        ++claim_call_count_;
        last_claimed_scope_ = processing_record.scope();
        throw_if_forced();

        const auto existing = find_entry(processing_record.scope());
        if (existing == records_.end()) {
            records_.push_back(processing_record);
            return IdempotencyClaimResult::claimed(processing_record);
        }
        if (existing->fingerprint() != processing_record.fingerprint()) {
            return IdempotencyClaimResult::fingerprint_mismatch(*existing);
        }
        switch (existing->status()) {
            case IdempotencyStatus::Processing:
                return IdempotencyClaimResult::existing_processing(*existing);
            case IdempotencyStatus::Succeeded:
                return IdempotencyClaimResult::existing_succeeded(*existing);
            case IdempotencyStatus::FailedPermanent:
                return IdempotencyClaimResult::existing_failed_permanently(*existing);
        }
        throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::InvalidRecord,
                                         "Stored idempotency record has an invalid state"};
    }

    [[nodiscard]] std::optional<haven::application::idempotency::IdempotencyRecord> find(
        const haven::application::idempotency::IdempotencyScope& scope) const override {
        const auto lock = std::scoped_lock{mutex_};
        ++find_call_count_;
        throw_if_forced();
        const auto existing = find_entry(scope);
        return existing == records_.end()
                   ? std::nullopt
                   : std::optional<haven::application::idempotency::IdempotencyRecord>{*existing};
    }

    void record_succeeded(
        const haven::application::idempotency::IdempotencyScope& scope,
        const haven::application::idempotency::IdempotencyFingerprint& expected_fingerprint,
        const haven::application::idempotency::CreateReservationResultSnapshot& snapshot) override {
        using namespace haven::application::idempotency;
        if (!snapshot.is_success()) {
            throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::InvalidSnapshot,
                                             "Success completion requires a successful snapshot"};
        }
        const auto lock = std::scoped_lock{mutex_};
        ++successful_completion_call_count_;
        if (force_successful_completion_failure_) {
            force_successful_completion_failure_ = false;
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence,
                "Forced idempotency completion failure"};
        }
        throw_if_forced();
        auto existing = require_matching_record(scope, expected_fingerprint);
        if (existing->status() == IdempotencyStatus::Succeeded && existing->result() == snapshot) {
            return;
        }
        if (existing->status() != IdempotencyStatus::Processing) {
            throw_terminal_conflict();
        }
        existing->record_succeeded(snapshot);
    }

    void record_failed_permanently(
        const haven::application::idempotency::IdempotencyScope& scope,
        const haven::application::idempotency::IdempotencyFingerprint& expected_fingerprint,
        const haven::application::idempotency::CreateReservationResultSnapshot& snapshot) override {
        using namespace haven::application::idempotency;
        if (snapshot.is_success()) {
            throw IdempotencyRepositoryError{
                IdempotencyRepositoryErrorCode::InvalidSnapshot,
                "Permanent failure completion requires a rejection snapshot"};
        }
        const auto lock = std::scoped_lock{mutex_};
        ++permanent_failure_completion_call_count_;
        throw_if_forced();
        auto existing = require_matching_record(scope, expected_fingerprint);
        if (existing->status() == IdempotencyStatus::FailedPermanent &&
            existing->result() == snapshot) {
            return;
        }
        if (existing->status() != IdempotencyStatus::Processing) {
            throw_terminal_conflict();
        }
        existing->record_failed_permanently(snapshot);
    }

    void force_repository_failure(const bool enabled = true) {
        const auto lock = std::scoped_lock{mutex_};
        force_repository_failure_ = enabled;
    }

    void force_successful_completion_failure(const bool enabled = true) {
        const auto lock = std::scoped_lock{mutex_};
        force_successful_completion_failure_ = enabled;
    }

    [[nodiscard]] std::size_t claim_call_count() const {
        const auto lock = std::scoped_lock{mutex_};
        return claim_call_count_;
    }
    [[nodiscard]] std::size_t find_call_count() const {
        const auto lock = std::scoped_lock{mutex_};
        return find_call_count_;
    }
    [[nodiscard]] std::size_t successful_completion_call_count() const {
        const auto lock = std::scoped_lock{mutex_};
        return successful_completion_call_count_;
    }
    [[nodiscard]] std::size_t permanent_failure_completion_call_count() const {
        const auto lock = std::scoped_lock{mutex_};
        return permanent_failure_completion_call_count_;
    }
    [[nodiscard]] std::size_t stored_record_count() const {
        const auto lock = std::scoped_lock{mutex_};
        return records_.size();
    }
    [[nodiscard]] std::optional<haven::application::idempotency::IdempotencyScope>
    last_claimed_scope() const {
        const auto lock = std::scoped_lock{mutex_};
        return last_claimed_scope_;
    }

private:
    using Records = std::vector<haven::application::idempotency::IdempotencyRecord>;

    [[nodiscard]] Records::iterator find_entry(
        const haven::application::idempotency::IdempotencyScope& scope) {
        return std::find_if(records_.begin(), records_.end(), [&scope](const auto& record) {
            return record.scope() == scope;
        });
    }
    [[nodiscard]] Records::const_iterator find_entry(
        const haven::application::idempotency::IdempotencyScope& scope) const {
        return std::find_if(records_.cbegin(), records_.cend(), [&scope](const auto& record) {
            return record.scope() == scope;
        });
    }

    [[nodiscard]] Records::iterator require_matching_record(
        const haven::application::idempotency::IdempotencyScope& scope,
        const haven::application::idempotency::IdempotencyFingerprint& expected_fingerprint) {
        using namespace haven::application::idempotency;
        auto existing = find_entry(scope);
        if (existing == records_.end()) {
            throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::MissingRecord,
                                             "Idempotency completion record is missing"};
        }
        if (existing->fingerprint() != expected_fingerprint) {
            throw IdempotencyRepositoryError{IdempotencyRepositoryErrorCode::FingerprintMismatch,
                                             "Idempotency completion fingerprint does not match"};
        }
        return existing;
    }

    [[noreturn]] static void throw_terminal_conflict() {
        throw haven::application::idempotency::IdempotencyRepositoryError{
            haven::application::idempotency::IdempotencyRepositoryErrorCode::TerminalConflict,
            "Terminal idempotency record cannot be overwritten"};
    }

    void throw_if_forced() const {
        if (force_repository_failure_) {
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence,
                "Forced idempotency repository failure"};
        }
    }

    mutable std::mutex mutex_;
    Records records_;
    bool force_repository_failure_{false};
    bool force_successful_completion_failure_{false};
    std::size_t claim_call_count_{};
    mutable std::size_t find_call_count_{};
    std::size_t successful_completion_call_count_{};
    std::size_t permanent_failure_completion_call_count_{};
    std::optional<haven::application::idempotency::IdempotencyScope> last_claimed_scope_;
};

}  // namespace haven::test::application::idempotency
