/**
 * @file outbox_publisher_worker_test.cpp
 * @brief Tests recurring Outbox worker lifecycle and serialization.
 */
#include "haven/runtime/outbox/outbox_publisher_worker.hpp"

#include "haven/application/repository_error.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace haven::runtime::outbox {
namespace {
using namespace std::chrono_literals;

class TestPublishCycle final : public haven::application::outbox::OutboxPublishCycle {
public:
    [[nodiscard]] haven::application::outbox::OutboxPublishCycleResult run_once(
        const std::size_t batch_size) const override {
        auto lock = std::unique_lock{mutex};
        ++calls;
        ++active;
        max_active = std::max(max_active, active);
        batch_sizes.push_back(batch_size);
        changed.notify_all();
        if (block)
            changed.wait(lock, [&] { return released; });
        --active;
        if (failures > 0) {
            --failures;
            changed.notify_all();
            throw haven::application::RepositoryError{
                haven::application::RepositoryErrorCode::Persistence, "scripted failure"};
        }
        changed.notify_all();
        return {.candidates_found = 1, .claims_acquired = 1, .published = 1};
    }

    bool wait_for_calls(const std::size_t expected,
                        const std::chrono::milliseconds timeout = 2s) const {
        auto lock = std::unique_lock{mutex};
        return changed.wait_for(lock, timeout, [&] { return calls >= expected; });
    }

    void unblock() {
        const auto lock = std::scoped_lock{mutex};
        released = true;
        changed.notify_all();
    }

    mutable std::mutex mutex;
    mutable std::condition_variable changed;
    mutable std::size_t calls{};
    mutable std::size_t active{};
    mutable std::size_t max_active{};
    mutable std::vector<std::size_t> batch_sizes;
    mutable std::size_t failures{};
    bool block{};
    bool released{};
};
}  // namespace

TEST(OutboxPublisherWorkerTest, ConstructionDoesNotStartCycle) {
    auto cycle = TestPublishCycle{};
    auto worker = OutboxPublisherWorker{cycle, 10, 1s};
    const auto lock = std::scoped_lock{cycle.mutex};
    EXPECT_EQ(cycle.calls, 0U);
}

TEST(OutboxPublisherWorkerTest, StartRunsPromptCycleWithConfiguredBatch) {
    auto cycle = TestPublishCycle{};
    auto worker = OutboxPublisherWorker{cycle, 17, 1s};
    worker.start();
    ASSERT_TRUE(cycle.wait_for_calls(1));
    worker.stop();
    const auto lock = std::scoped_lock{cycle.mutex};
    EXPECT_EQ(cycle.batch_sizes, std::vector<std::size_t>{17});
}

TEST(OutboxPublisherWorkerTest, RepeatsSequentiallyWithoutOverlap) {
    auto cycle = TestPublishCycle{};
    auto worker = OutboxPublisherWorker{cycle, 10, 1ms};
    worker.start();
    ASSERT_TRUE(cycle.wait_for_calls(2));
    worker.stop();
    const auto lock = std::scoped_lock{cycle.mutex};
    EXPECT_GE(cycle.calls, 2U);
    EXPECT_EQ(cycle.max_active, 1U);
}

TEST(OutboxPublisherWorkerTest, BlockedCycleCannotOverlapAndStopJoinsIt) {
    auto cycle = TestPublishCycle{};
    cycle.block = true;
    auto worker = OutboxPublisherWorker{cycle, 10, 1ms};
    worker.start();
    ASSERT_TRUE(cycle.wait_for_calls(1));
    auto stopping = std::async(std::launch::async, [&] { worker.stop(); });
    EXPECT_EQ(stopping.wait_for(50ms), std::future_status::timeout);
    {
        const auto lock = std::scoped_lock{cycle.mutex};
        EXPECT_EQ(cycle.calls, 1U);
        EXPECT_EQ(cycle.max_active, 1U);
    }
    cycle.unblock();
    EXPECT_EQ(stopping.wait_for(2s), std::future_status::ready);
}

TEST(OutboxPublisherWorkerTest, StopInterruptsLongPollingWaitAndIsIdempotent) {
    auto cycle = TestPublishCycle{};
    auto worker = OutboxPublisherWorker{cycle, 10, std::chrono::hours{1}};
    worker.start();
    ASSERT_TRUE(cycle.wait_for_calls(1));
    const auto start = std::chrono::steady_clock::now();
    worker.stop();
    worker.stop();
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
}

TEST(OutboxPublisherWorkerTest, CycleFailureDoesNotTerminateWorker) {
    auto cycle = TestPublishCycle{};
    cycle.failures = 1;
    auto worker = OutboxPublisherWorker{cycle, 10, 1ms};
    worker.start();
    ASSERT_TRUE(cycle.wait_for_calls(2));
    worker.stop();
}

TEST(OutboxPublisherWorkerTest, DestructorStopsWorker) {
    auto cycle = TestPublishCycle{};
    {
        auto worker = OutboxPublisherWorker{cycle, 10, std::chrono::hours{1}};
        worker.start();
        ASSERT_TRUE(cycle.wait_for_calls(1));
    }
    const auto lock = std::scoped_lock{cycle.mutex};
    EXPECT_EQ(cycle.active, 0U);
}

TEST(OutboxPublisherWorkerTest, RejectsInvalidRuntimeConfiguration) {
    auto cycle = TestPublishCycle{};
    EXPECT_THROW(OutboxPublisherWorker(cycle, 0, 1ms), std::invalid_argument);
    EXPECT_THROW(OutboxPublisherWorker(cycle, 1, 0ms), std::invalid_argument);
    EXPECT_THROW(OutboxPublisherWorker(cycle, 1, -1ms), std::invalid_argument);
}

TEST(OutboxPublisherWorkerTest, RepeatedStartFailsClearly) {
    auto cycle = TestPublishCycle{};
    auto worker = OutboxPublisherWorker{cycle, 10, 1s};
    worker.start();
    ASSERT_TRUE(cycle.wait_for_calls(1));
    EXPECT_THROW(worker.start(), std::logic_error);
    worker.stop();
}

}  // namespace haven::runtime::outbox
