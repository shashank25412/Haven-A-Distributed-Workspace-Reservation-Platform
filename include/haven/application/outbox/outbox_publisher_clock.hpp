/**
 * @file outbox_publisher_clock.hpp
 * @brief Declares the clock used to timestamp acknowledged Outbox publication.
 */
#pragma once

#include <chrono>

namespace haven::application::outbox {

class OutboxPublisherClock {
public:
    virtual ~OutboxPublisherClock() = default;
    [[nodiscard]] virtual std::chrono::system_clock::time_point now() const = 0;
};

}  // namespace haven::application::outbox
