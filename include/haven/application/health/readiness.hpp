/** @file readiness.hpp @brief Declares bounded dependency readiness orchestration. */
#pragma once

#include <functional>

namespace haven::application::health {
enum class ReadinessStatus { ready, not_ready };
enum class DependencyStatus { up, down, disabled };

class ReadinessProbe {
public:
    virtual ~ReadinessProbe() = default;
    [[nodiscard]] virtual bool is_ready() const = 0;
};

class FunctionReadinessProbe final : public ReadinessProbe {
public:
    explicit FunctionReadinessProbe(std::function<bool()> probe);
    [[nodiscard]] bool is_ready() const override;

private:
    std::function<bool()> probe_;
};

struct ReadinessResult final {
    ReadinessStatus status;
    DependencyStatus couchbase;
    DependencyStatus redis;
    DependencyStatus kafka;
    DependencyStatus outbox_publisher;
};

class ReadinessService final {
public:
    ReadinessService(ReadinessProbe& couchbase,
                     ReadinessProbe* redis,
                     ReadinessProbe* kafka,
                     ReadinessProbe* outbox_publisher) noexcept;
    [[nodiscard]] ReadinessResult check() const noexcept;

private:
    ReadinessProbe& couchbase_;
    ReadinessProbe* redis_;
    ReadinessProbe* kafka_;
    ReadinessProbe* outbox_publisher_;
};

[[nodiscard]] const char* to_string(ReadinessStatus status) noexcept;
[[nodiscard]] const char* to_string(DependencyStatus status) noexcept;
}  // namespace haven::application::health
