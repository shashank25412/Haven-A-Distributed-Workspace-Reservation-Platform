/**
 * @file system_outbox_publisher_clock.hpp
 * @brief Declares the production system-clock publisher adapter.
 */
#pragma once

#include "haven/application/outbox/outbox_publisher_clock.hpp"

namespace haven::application::outbox {

class SystemOutboxPublisherClock final : public OutboxPublisherClock {
public:
    [[nodiscard]] std::chrono::system_clock::time_point now() const override;
};

}  // namespace haven::application::outbox
