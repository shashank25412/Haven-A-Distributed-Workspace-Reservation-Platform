/**
 * @file outbox_publisher.cpp
 * @brief Implements one bounded Outbox publishing cycle.
 */
#include "haven/application/outbox/outbox_publisher.hpp"

#include "haven/application/outbox/message_publish_error.hpp"
#include "haven/application/outbox/metrics/outbox_publisher_metrics.hpp"
#include "haven/application/repository_error.hpp"
#include "haven/logging/logging.hpp"

#include <chrono>
#include <exception>
#include <stdexcept>

namespace haven::application::outbox {

OutboxPublisher::OutboxPublisher(OutboxRepository& repository,
                                 OutboxMessageProducer& producer,
                                 const OutboxPublisherClock& clock,
                                 observability::metrics::MetricsRecorder& metrics_recorder) noexcept
    : repository_(repository),
      producer_(producer),
      clock_(clock),
      metrics_recorder_(metrics_recorder) {}

namespace {
observability::metrics::MetricLabels labels(const metrics::CycleOutcome outcome) {
    return observability::metrics::MetricLabels{metrics::outcome_label(outcome)};
}
observability::metrics::MetricLabels labels(const metrics::RecordOutcome outcome) {
    return observability::metrics::MetricLabels{metrics::outcome_label(outcome)};
}
}  // namespace

OutboxPublishCycleResult OutboxPublisher::run_once(const std::size_t batch_size) const {
    HVN_TRACE_SCOPE();
    const auto started_at = std::chrono::steady_clock::now();
    auto outcome = metrics::CycleOutcome::completed;
    auto failure = std::exception_ptr{};
    try {
        auto result = run_cycle(batch_size);
        try {
            metrics_recorder_.increment_counter(
                metrics::cycles_metric_name(), 1.0, labels(outcome));
        } catch (...) {}
        try {
            metrics_recorder_.observe_duration(
                metrics::cycle_duration_metric_name(),
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - started_at),
                labels(outcome));
        } catch (...) {}
        return result;
    } catch (const haven::application::RepositoryError&) {
        outcome = metrics::CycleOutcome::repository_failure;
        failure = std::current_exception();
    } catch (...) {
        outcome = metrics::CycleOutcome::unexpected_failure;
        failure = std::current_exception();
    }
    try {
        metrics_recorder_.increment_counter(metrics::cycles_metric_name(), 1.0, labels(outcome));
    } catch (...) {}
    try {
        metrics_recorder_.observe_duration(metrics::cycle_duration_metric_name(),
                                           std::chrono::duration_cast<std::chrono::microseconds>(
                                               std::chrono::steady_clock::now() - started_at),
                                           labels(outcome));
    } catch (...) {}
    std::rethrow_exception(failure);
}

OutboxPublishCycleResult OutboxPublisher::run_cycle(const std::size_t batch_size) const {
    if (batch_size == 0)
        throw std::invalid_argument("Outbox publisher batch size must be positive");
    HVN_DEBUG_LOG("Starting Outbox publish cycle with batch size ", batch_size);

    auto result = OutboxPublishCycleResult{};
    std::vector<LoadedOutboxMessage> candidates;
    try {
        candidates = repository_.find_pending(batch_size);
    } catch (const haven::application::RepositoryError&) {
        HVN_ERROR_LOG("Outbox pending query prevented publish-cycle progress");
        throw;
    }
    result.candidates_found = candidates.size();
    try {
        metrics_recorder_.increment_counter(
            metrics::records_discovered_metric_name(), static_cast<double>(candidates.size()), {});
    } catch (...) {}

    const auto record_outcome = [this](const metrics::RecordOutcome outcome) noexcept {
        try {
            metrics_recorder_.increment_counter(
                metrics::record_attempts_metric_name(), 1.0, labels(outcome));
        } catch (...) {}
    };

    for (const auto& candidate : candidates) {
        const auto& pending = candidate.aggregate();
        std::optional<LoadedOutboxMessage> claimed;
        try {
            claimed = repository_.claim(
                pending.organization_id, pending.event_id, candidate.persistence_token());
        } catch (const haven::application::RepositoryError&) {
            record_outcome(metrics::RecordOutcome::claim_failed);
            HVN_ERROR_LOG("Outbox claim prevented publish-cycle progress for event ",
                          pending.event_id.value());
            throw;
        } catch (...) {
            record_outcome(metrics::RecordOutcome::record_failed);
            throw;
        }
        if (!claimed) {
            record_outcome(metrics::RecordOutcome::claim_not_acquired);
            ++result.claims_lost;
            HVN_DEBUG_LOG("Outbox claim lost for event ", pending.event_id.value());
            continue;
        }
        ++result.claims_acquired;
        const auto& message = claimed->aggregate();
        try {
            producer_.publish(message);
        } catch (const MessagePublishError&) {
            HVN_WARN_LOG("Outbox publication failed for event ", message.event_id.value());
            try {
                static_cast<void>(repository_.release_for_retry(
                    message.organization_id, message.event_id, claimed->persistence_token()));
                ++result.released_for_retry;
                record_outcome(metrics::RecordOutcome::publish_failed_released);
            } catch (const haven::application::RepositoryError&) {
                ++result.release_failures;
                record_outcome(metrics::RecordOutcome::publish_failed_release_failed);
                HVN_WARN_LOG("Outbox retry release failed for event ", message.event_id.value());
            } catch (...) {
                record_outcome(metrics::RecordOutcome::record_failed);
                throw;
            }
            continue;
        } catch (...) {
            record_outcome(metrics::RecordOutcome::record_failed);
            throw;
        }

        try {
            static_cast<void>(repository_.mark_published(message.organization_id,
                                                         message.event_id,
                                                         claimed->persistence_token(),
                                                         clock_.now()));
            ++result.published;
            record_outcome(metrics::RecordOutcome::published);
            HVN_DEBUG_LOG("Outbox message published for event ", message.event_id.value());
        } catch (const haven::application::RepositoryError&) {
            ++result.completion_failures;
            record_outcome(metrics::RecordOutcome::published_mark_failed);
            HVN_WARN_LOG("Outbox completion failed after acknowledgement for event ",
                         message.event_id.value());
        } catch (...) {
            record_outcome(metrics::RecordOutcome::record_failed);
            throw;
        }
    }

    HVN_DEBUG_LOG("Completed Outbox publish cycle: candidates=",
                  result.candidates_found,
                  ", claimed=",
                  result.claims_acquired,
                  ", published=",
                  result.published);
    return result;
}

}  // namespace haven::application::outbox
