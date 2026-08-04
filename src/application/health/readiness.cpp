/** @file readiness.cpp @brief Implements bounded dependency readiness orchestration. */
#include "haven/application/health/readiness.hpp"

#include <stdexcept>
#include <utility>

namespace haven::application::health {
FunctionReadinessProbe::FunctionReadinessProbe(std::function<bool()> probe)
    : probe_(std::move(probe)) {
    if (!probe_)
        throw std::invalid_argument("Readiness callback is empty");
}
bool FunctionReadinessProbe::is_ready() const {
    return probe_();
}

ReadinessService::ReadinessService(ReadinessProbe& couchbase,
                                   ReadinessProbe* redis,
                                   ReadinessProbe* kafka,
                                   ReadinessProbe* outbox_publisher) noexcept
    : couchbase_(couchbase), redis_(redis), kafka_(kafka), outbox_publisher_(outbox_publisher) {}

namespace {
DependencyStatus evaluate(const ReadinessProbe* probe) noexcept {
    if (!probe)
        return DependencyStatus::disabled;
    try {
        return probe->is_ready() ? DependencyStatus::up : DependencyStatus::down;
    } catch (...) {
        return DependencyStatus::down;
    }
}
}  // namespace

ReadinessResult ReadinessService::check() const noexcept {
    const auto couchbase = evaluate(&couchbase_);
    const auto redis = evaluate(redis_);
    const auto kafka = evaluate(kafka_);
    const auto worker = evaluate(outbox_publisher_);
    const auto required_up = [](const DependencyStatus value) {
        return value == DependencyStatus::up || value == DependencyStatus::disabled;
    };
    return {.status = couchbase == DependencyStatus::up && required_up(redis) &&
                              required_up(kafka) && required_up(worker)
                          ? ReadinessStatus::ready
                          : ReadinessStatus::not_ready,
            .couchbase = couchbase,
            .redis = redis,
            .kafka = kafka,
            .outbox_publisher = worker};
}
const char* to_string(const ReadinessStatus status) noexcept {
    return status == ReadinessStatus::ready ? "ready" : "not_ready";
}
const char* to_string(const DependencyStatus status) noexcept {
    switch (status) {
        case DependencyStatus::up:
            return "up";
        case DependencyStatus::down:
            return "down";
        case DependencyStatus::disabled:
            return "disabled";
    }
    return "down";
}
}  // namespace haven::application::health
