/**
    @file system_outbox_publisher_clock.cpp
    @brief Implements the system publisher clock.
*/
#include "haven/application/outbox/system_outbox_publisher_clock.hpp"

namespace haven::application::outbox {

std::chrono::system_clock::time_point SystemOutboxPublisherClock::now() const {
    return std::chrono::system_clock::now();
}

}  // namespace haven::application::outbox
