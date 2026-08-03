/**
 * @file outbox_publisher_worker.hpp
 * @brief Declares the server-owned recurring Outbox publisher worker.
 */
#pragma once

#include "haven/application/outbox/outbox_publish_cycle.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>

namespace haven::runtime::outbox {

/**
 * @brief Runs one publisher cycle promptly, then repeats using fixed-delay scheduling.
 *
 * One owned jthread prevents overlap. Construction is inert; destruction requests stop and joins.
 */
class OutboxPublisherWorker final {
public:
    OutboxPublisherWorker(haven::application::outbox::OutboxPublishCycle& publisher,
                          std::size_t batch_size,
                          std::chrono::milliseconds poll_interval);
    ~OutboxPublisherWorker();

    OutboxPublisherWorker(const OutboxPublisherWorker&) = delete;
    OutboxPublisherWorker& operator=(const OutboxPublisherWorker&) = delete;
    OutboxPublisherWorker(OutboxPublisherWorker&&) = delete;
    OutboxPublisherWorker& operator=(OutboxPublisherWorker&&) = delete;

    void start();
    void stop() noexcept;

private:
    void run(std::stop_token stop_token) noexcept;

    haven::application::outbox::OutboxPublishCycle& publisher_;
    std::size_t batch_size_;
    std::chrono::milliseconds poll_interval_;
    std::mutex lifecycle_mutex_;
    std::mutex wait_mutex_;
    std::condition_variable_any wake_;
    std::jthread thread_;
};

}  // namespace haven::runtime::outbox
