/**
 * @file outbox_publisher_worker.cpp
 * @brief Implements the stop-aware fixed-delay Outbox publisher worker.
 */
#include "haven/runtime/outbox/outbox_publisher_worker.hpp"

#include "haven/application/outbox/metrics/outbox_publisher_metrics.hpp"
#include "haven/application/repository_error.hpp"
#include "haven/logging/logging.hpp"

#include <exception>
#include <stdexcept>

namespace haven::runtime::outbox {

OutboxPublisherWorker::OutboxPublisherWorker(
    haven::application::outbox::OutboxPublishCycle& publisher,
    const std::size_t batch_size,
    const std::chrono::milliseconds poll_interval,
    haven::application::observability::metrics::MetricsRecorder& metrics_recorder)
    : publisher_(publisher),
      batch_size_(batch_size),
      poll_interval_(poll_interval),
      metrics_recorder_(metrics_recorder) {
    if (batch_size == 0)
        throw std::invalid_argument("Outbox publisher worker batch size must be positive");
    if (poll_interval <= std::chrono::milliseconds::zero())
        throw std::invalid_argument("Outbox publisher worker poll interval must be positive");
}

OutboxPublisherWorker::~OutboxPublisherWorker() {
    stop();
}

void OutboxPublisherWorker::start() {
    const auto lock = std::scoped_lock{lifecycle_mutex_};
    if (thread_.joinable())
        throw std::logic_error("Outbox publisher worker is already started");
    try {
        thread_ = std::jthread{[this](const std::stop_token token) { run(token); }};
    } catch (...) {
        record_running(0.0);
        throw;
    }
    HVN_INFO_LOG("Outbox publisher worker started");
}

void OutboxPublisherWorker::stop() noexcept {
    const auto lock = std::scoped_lock{lifecycle_mutex_};
    if (!thread_.joinable())
        return;
    thread_.request_stop();
    wake_.notify_all();
    try {
        thread_.join();
        HVN_INFO_LOG("Outbox publisher worker stopped");
    } catch (const std::exception& error) {
        HVN_ERROR_LOG("Failed to join Outbox publisher worker: ", error.what());
    }
}

void OutboxPublisherWorker::run(const std::stop_token stop_token) noexcept {
    running_.store(true);
    record_running(1.0);
    while (!stop_token.stop_requested()) {
        try {
            const auto result = publisher_.run_once(batch_size_);
            HVN_DEBUG_LOG("Outbox worker cycle: candidates=",
                          result.candidates_found,
                          ", acquired=",
                          result.claims_acquired,
                          ", lost=",
                          result.claims_lost,
                          ", published=",
                          result.published,
                          ", released=",
                          result.released_for_retry,
                          ", completion_failures=",
                          result.completion_failures,
                          ", release_failures=",
                          result.release_failures);
        } catch (const haven::application::RepositoryError& error) {
            HVN_WARN_LOG("Outbox publisher cycle repository failure: ", error.what());
        } catch (const std::exception& error) {
            HVN_ERROR_LOG("Outbox publisher cycle failed unexpectedly: ", error.what());
        } catch (...) {
            HVN_ERROR_LOG("Outbox publisher cycle failed with an unknown error");
        }
        if (stop_token.stop_requested())
            break;
        auto lock = std::unique_lock{wait_mutex_};
        static_cast<void>(wake_.wait_for(lock, stop_token, poll_interval_, [] { return false; }));
    }
    record_running(0.0);
    running_.store(false);
}

bool OutboxPublisherWorker::is_running() const noexcept {
    return running_.load();
}

void OutboxPublisherWorker::record_running(const double value) noexcept {
    try {
        metrics_recorder_.set_gauge(
            haven::application::outbox::metrics::worker_running_metric_name(), value, {});
    } catch (...) {
        // Observability must never alter worker lifecycle behavior.
    }
}

}  // namespace haven::runtime::outbox
