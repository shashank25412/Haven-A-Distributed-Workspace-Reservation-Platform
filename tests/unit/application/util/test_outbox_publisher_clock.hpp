/**
 * @file test_outbox_publisher_clock.hpp
 * @brief Provides a deterministic clock for Outbox publisher tests.
 */
#pragma once

#include "haven/application/outbox/outbox_publisher_clock.hpp"

namespace haven::application::outbox::test {

class TestOutboxPublisherClock final : public OutboxPublisherClock {
public:
    [[nodiscard]] std::chrono::system_clock::time_point now() const override {
        ++calls;
        return current;
    }

    std::chrono::system_clock::time_point current{};
    mutable std::size_t calls{};
};

}  // namespace haven::application::outbox::test
