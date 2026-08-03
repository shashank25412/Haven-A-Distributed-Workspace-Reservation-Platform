/**
 * @file outbox_publish_cycle.hpp
 * @brief Declares one bounded Outbox publishing-cycle behavior.
 */
#pragma once

#include <cstddef>

namespace haven::application::outbox {

struct OutboxPublishCycleResult final {
    std::size_t candidates_found{};
    std::size_t claims_acquired{};
    std::size_t claims_lost{};
    std::size_t published{};
    std::size_t released_for_retry{};
    std::size_t completion_failures{};
    std::size_t release_failures{};

    bool operator==(const OutboxPublishCycleResult&) const = default;
};

class OutboxPublishCycle {
public:
    virtual ~OutboxPublishCycle() = default;
    [[nodiscard]] virtual OutboxPublishCycleResult run_once(std::size_t batch_size) const = 0;
};

}  // namespace haven::application::outbox
