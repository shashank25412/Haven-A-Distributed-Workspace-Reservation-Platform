/** @file readiness_test.cpp @brief Tests dependency readiness orchestration. */
#include "haven/application/health/readiness.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::application::health {
namespace {
class Probe final : public ReadinessProbe {
public:
    [[nodiscard]] bool is_ready() const override {
        ++calls;
        if (throws)
            throw std::runtime_error{"probe failure"};
        return ready;
    }

    bool ready{true};
    bool throws{};
    mutable int calls{};
};
}  // namespace

TEST(ReadinessServiceTest, ReportsReadyWhenAllEnabledDependenciesAreUp) {
    auto couchbase = Probe{};
    auto redis = Probe{};
    auto kafka = Probe{};
    auto worker = Probe{};

    const auto result = ReadinessService{couchbase, &redis, &kafka, &worker}.check();

    EXPECT_EQ(result.status, ReadinessStatus::ready);
    EXPECT_EQ(result.couchbase, DependencyStatus::up);
    EXPECT_EQ(result.redis, DependencyStatus::up);
    EXPECT_EQ(result.kafka, DependencyStatus::up);
    EXPECT_EQ(result.outbox_publisher, DependencyStatus::up);
}

TEST(ReadinessServiceTest, DisabledOptionalDependenciesDoNotBlockReadiness) {
    auto couchbase = Probe{};

    const auto result = ReadinessService{couchbase, nullptr, nullptr, nullptr}.check();

    EXPECT_EQ(result.status, ReadinessStatus::ready);
    EXPECT_EQ(result.couchbase, DependencyStatus::up);
    EXPECT_EQ(result.redis, DependencyStatus::disabled);
    EXPECT_EQ(result.kafka, DependencyStatus::disabled);
    EXPECT_EQ(result.outbox_publisher, DependencyStatus::disabled);
}

TEST(ReadinessServiceTest, AnyEnabledDependencyDownMakesServiceNotReady) {
    auto couchbase = Probe{};
    auto redis = Probe{};
    auto kafka = Probe{};
    auto worker = Probe{};
    redis.ready = false;

    auto result = ReadinessService{couchbase, &redis, &kafka, &worker}.check();
    EXPECT_EQ(result.status, ReadinessStatus::not_ready);
    EXPECT_EQ(result.redis, DependencyStatus::down);

    redis.ready = true;
    kafka.ready = false;
    result = ReadinessService{couchbase, &redis, &kafka, &worker}.check();
    EXPECT_EQ(result.status, ReadinessStatus::not_ready);

    kafka.ready = true;
    worker.ready = false;
    result = ReadinessService{couchbase, &redis, &kafka, &worker}.check();
    EXPECT_EQ(result.status, ReadinessStatus::not_ready);

    worker.ready = true;
    couchbase.ready = false;
    result = ReadinessService{couchbase, &redis, &kafka, &worker}.check();
    EXPECT_EQ(result.status, ReadinessStatus::not_ready);
}

TEST(ReadinessServiceTest, ProbeExceptionsAreDownAndDoNotSkipOtherChecks) {
    auto couchbase = Probe{};
    auto redis = Probe{};
    auto kafka = Probe{};
    auto worker = Probe{};
    redis.throws = true;

    const auto result = ReadinessService{couchbase, &redis, &kafka, &worker}.check();

    EXPECT_EQ(result.status, ReadinessStatus::not_ready);
    EXPECT_EQ(result.redis, DependencyStatus::down);
    EXPECT_EQ(couchbase.calls, 1);
    EXPECT_EQ(redis.calls, 1);
    EXPECT_EQ(kafka.calls, 1);
    EXPECT_EQ(worker.calls, 1);
}

TEST(ReadinessServiceTest, FunctionProbeRejectsEmptyCallback) {
    EXPECT_THROW(FunctionReadinessProbe({}), std::invalid_argument);
}

}  // namespace haven::application::health
