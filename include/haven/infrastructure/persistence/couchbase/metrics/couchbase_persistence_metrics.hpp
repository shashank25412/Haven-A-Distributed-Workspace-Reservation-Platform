/**
 * @file couchbase_persistence_metrics.hpp
 * @brief Defines bounded Couchbase persistence metrics and recording support.
 */
#pragma once

#include "haven/application/observability/metrics/metrics_recorder.hpp"
#include "haven/application/repository_error.hpp"

#include <chrono>
#include <functional>
#include <string_view>
#include <type_traits>

namespace haven::infrastructure::persistence::couchbase::metrics {

enum class Repository {
    resource,
    reservation,
    idempotency,
    outbox,
    reservation_creation_store,
    reservation_creation_event_store
};
enum class Operation {
    find_by_id,
    find_active_by_type,
    find_by_creator,
    find_pending_approvals,
    find_by_resource_and_interval,
    has_conflict,
    has_conflict_excluding,
    insert,
    update,
    claim,
    find,
    record_succeeded,
    record_failed_permanently,
    find_pending,
    mark_published,
    release_for_retry,
    create_reservation_with_outbox,
    contains_all
};
enum class Outcome {
    success,
    already_exists,
    concurrency_conflict,
    timeout,
    persistence_failed,
    unexpected_failure
};
enum class TransactionOutcome { committed, conflict, failed, unexpected_failure };

/** Counter of top-level repository calls; bounded repository, operation, and outcome labels only.
 */
[[nodiscard]] const application::observability::metrics::MetricName& operations_metric_name();
/** Complete repository-call duration with the same bounded labels as the operation counter. */
[[nodiscard]] const application::observability::metrics::MetricName&
operation_duration_metric_name();
/** Genuine persistence CAS conflicts, labelled only by repository and operation. */
[[nodiscard]] const application::observability::metrics::MetricName& conflicts_metric_name();
/** Top-level Reservation plus Outbox transactions, not SDK callback retries. */
[[nodiscard]] const application::observability::metrics::MetricName& transactions_metric_name();
/** Complete top-level transaction duration, including any SDK-managed retries. */
[[nodiscard]] const application::observability::metrics::MetricName&
transaction_duration_metric_name();
[[nodiscard]] std::string_view value(Repository value) noexcept;
[[nodiscard]] std::string_view value(Operation value) noexcept;
[[nodiscard]] std::string_view value(Outcome value) noexcept;
[[nodiscard]] std::string_view value(TransactionOutcome value) noexcept;

/**
 * Records one terminal metric set around each concrete repository invocation.
 * CAS conflicts describe persistence concurrency and are independent of Domain Version checks.
 * Application business outcomes are intentionally classified by their own higher-layer metrics.
 */
class OperationMetrics final {
public:
    explicit OperationMetrics(
        application::observability::metrics::MetricsRecorder& recorder) noexcept;

    template <typename Function>
    decltype(auto) record(Repository repository, Operation operation, Function&& function) const {
        const auto started_at = std::chrono::steady_clock::now();
        try {
            if constexpr (std::is_void_v<std::invoke_result_t<Function>>) {
                std::invoke(std::forward<Function>(function));
                record_operation(repository, operation, Outcome::success, started_at);
                return;
            } else {
                auto result = std::invoke(std::forward<Function>(function));
                record_operation(repository, operation, Outcome::success, started_at);
                return result;
            }
        } catch (const application::RepositoryError& error) {
            record_operation(repository, operation, outcome(error.code()), started_at);
            throw;
        } catch (...) {
            record_operation(repository, operation, Outcome::unexpected_failure, started_at);
            throw;
        }
    }

    template <typename Function>
    decltype(auto) record_transaction(Function&& function) const {
        const auto started_at = std::chrono::steady_clock::now();
        try {
            auto result = std::invoke(std::forward<Function>(function));
            record_operation(Repository::reservation_creation_store,
                             Operation::create_reservation_with_outbox,
                             Outcome::success,
                             started_at);
            record_transaction_terminal(TransactionOutcome::committed, started_at);
            return result;
        } catch (const application::RepositoryError& error) {
            const auto operation_outcome = outcome(error.code());
            record_operation(Repository::reservation_creation_store,
                             Operation::create_reservation_with_outbox,
                             operation_outcome,
                             started_at);
            record_transaction_terminal(transaction_outcome(error.code()), started_at);
            throw;
        } catch (...) {
            record_operation(Repository::reservation_creation_store,
                             Operation::create_reservation_with_outbox,
                             Outcome::unexpected_failure,
                             started_at);
            record_transaction_terminal(TransactionOutcome::unexpected_failure, started_at);
            throw;
        }
    }

private:
    [[nodiscard]] static Outcome outcome(application::RepositoryErrorCode code) noexcept;
    [[nodiscard]] static TransactionOutcome transaction_outcome(
        application::RepositoryErrorCode code) noexcept;
    void record_operation(Repository repository,
                          Operation operation,
                          Outcome outcome,
                          std::chrono::steady_clock::time_point started_at) const noexcept;
    void record_transaction_terminal(
        TransactionOutcome outcome,
        std::chrono::steady_clock::time_point started_at) const noexcept;

    application::observability::metrics::MetricsRecorder& recorder_;
};

}  // namespace haven::infrastructure::persistence::couchbase::metrics
