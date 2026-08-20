/** @file couchbase_persistence_metrics.cpp @brief Implements Couchbase persistence metrics. */
#include "haven/infrastructure/persistence/couchbase/metrics/couchbase_persistence_metrics.hpp"

#include "haven/application/observability/metrics/metric_label.hpp"

#include <string>

namespace haven::infrastructure::persistence::couchbase::metrics {
using application::observability::metrics::MetricLabel;
using application::observability::metrics::MetricLabels;
using application::observability::metrics::MetricName;

const MetricName& operations_metric_name() {
    static const MetricName n{"haven_couchbase_repository_operations_total"};
    return n;
}
const MetricName& operation_duration_metric_name() {
    static const MetricName n{"haven_couchbase_repository_operation_duration_seconds"};
    return n;
}
const MetricName& conflicts_metric_name() {
    static const MetricName n{"haven_couchbase_concurrency_conflicts_total"};
    return n;
}
const MetricName& transactions_metric_name() {
    static const MetricName n{"haven_couchbase_reservation_creation_transactions_total"};
    return n;
}
const MetricName& transaction_duration_metric_name() {
    static const MetricName n{"haven_couchbase_reservation_creation_transaction_duration_seconds"};
    return n;
}

std::string_view value(const Repository v) noexcept {
    switch (v) {
        case Repository::resource:
            return "resource";
        case Repository::reservation:
            return "reservation";
        case Repository::idempotency:
            return "idempotency";
        case Repository::outbox:
            return "outbox";
        case Repository::reservation_creation_store:
            return "reservation_creation_store";
        case Repository::reservation_creation_event_store:
            return "reservation_creation_event_store";
    }
    return "reservation_creation_store";
}
std::string_view value(const Operation v) noexcept {
    switch (v) {
        case Operation::find_by_id:
            return "find_by_id";
        case Operation::find_active_by_type:
            return "find_active_by_type";
        case Operation::find_by_creator:
            return "find_by_creator";
        case Operation::find_pending_approvals:
            return "find_pending_approvals";
        case Operation::find_decided_approvals:
            return "find_decided_approvals";
        case Operation::find_all:
            return "find_all";
        case Operation::find_by_resource_and_interval:
            return "find_by_resource_and_interval";
        case Operation::has_conflict:
            return "has_conflict";
        case Operation::has_conflict_excluding:
            return "has_conflict_excluding";
        case Operation::reserved_unit_count:
            return "reserved_unit_count";
        case Operation::insert:
            return "insert";
        case Operation::update:
            return "update";
        case Operation::claim:
            return "claim";
        case Operation::find:
            return "find";
        case Operation::record_succeeded:
            return "record_succeeded";
        case Operation::record_failed_permanently:
            return "record_failed_permanently";
        case Operation::find_pending:
            return "find_pending";
        case Operation::mark_published:
            return "mark_published";
        case Operation::release_for_retry:
            return "release_for_retry";
        case Operation::create_reservation_with_outbox:
            return "create_reservation_with_outbox";
        case Operation::contains_all:
            return "contains_all";
    }
    return "find_by_id";
}
std::string_view value(const Outcome v) noexcept {
    switch (v) {
        case Outcome::success:
            return "success";
        case Outcome::already_exists:
            return "already_exists";
        case Outcome::concurrency_conflict:
            return "concurrency_conflict";
        case Outcome::timeout:
            return "timeout";
        case Outcome::persistence_failed:
            return "persistence_failed";
        case Outcome::unexpected_failure:
            return "unexpected_failure";
    }
    return "unexpected_failure";
}
std::string_view value(const TransactionOutcome v) noexcept {
    switch (v) {
        case TransactionOutcome::committed:
            return "committed";
        case TransactionOutcome::conflict:
            return "conflict";
        case TransactionOutcome::failed:
            return "failed";
        case TransactionOutcome::unexpected_failure:
            return "unexpected_failure";
    }
    return "unexpected_failure";
}

OperationMetrics::OperationMetrics(
    application::observability::metrics::MetricsRecorder& recorder) noexcept
    : recorder_(recorder) {}
Outcome OperationMetrics::outcome(const application::RepositoryErrorCode code) noexcept {
    switch (code) {
        case application::RepositoryErrorCode::AlreadyExists:
            return Outcome::already_exists;
        case application::RepositoryErrorCode::ConcurrencyConflict:
            return Outcome::concurrency_conflict;
        case application::RepositoryErrorCode::Timeout:
            return Outcome::timeout;
        case application::RepositoryErrorCode::Authentication:
        case application::RepositoryErrorCode::Authorization:
        case application::RepositoryErrorCode::Persistence:
            return Outcome::persistence_failed;
    }
    return Outcome::persistence_failed;
}
TransactionOutcome OperationMetrics::transaction_outcome(
    const application::RepositoryErrorCode code) noexcept {
    switch (code) {
        case application::RepositoryErrorCode::AlreadyExists:
        case application::RepositoryErrorCode::ConcurrencyConflict:
            return TransactionOutcome::conflict;
        case application::RepositoryErrorCode::Timeout:
            return TransactionOutcome::failed;
        default:
            return TransactionOutcome::failed;
    }
}

void OperationMetrics::record_operation(
    const Repository repository,
    const Operation operation,
    const Outcome outcome_value,
    const std::chrono::steady_clock::time_point started_at) const noexcept {
    const auto labels = [&] {
        return MetricLabels{{"repository", std::string{value(repository)}},
                            {"operation", std::string{value(operation)}},
                            {"outcome", std::string{value(outcome_value)}}};
    };
    try {
        recorder_.increment_counter(operations_metric_name(), 1.0, labels());
    } catch (...) {}
    try {
        recorder_.observe_duration(operation_duration_metric_name(),
                                   std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::steady_clock::now() - started_at),
                                   labels());
    } catch (...) {}
    if (outcome_value == Outcome::concurrency_conflict) {
        try {
            recorder_.increment_counter(conflicts_metric_name(),
                                        1.0,
                                        {{"repository", std::string{value(repository)}},
                                         {"operation", std::string{value(operation)}}});
        } catch (...) {}
    }
}
void OperationMetrics::record_transaction_terminal(
    const TransactionOutcome outcome_value,
    const std::chrono::steady_clock::time_point started_at) const noexcept {
    const auto labels = [&] {
        return MetricLabels{{"outcome", std::string{value(outcome_value)}}};
    };
    try {
        recorder_.increment_counter(transactions_metric_name(), 1.0, labels());
    } catch (...) {}
    try {
        recorder_.observe_duration(transaction_duration_metric_name(),
                                   std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::steady_clock::now() - started_at),
                                   labels());
    } catch (...) {}
}
}  // namespace haven::infrastructure::persistence::couchbase::metrics
