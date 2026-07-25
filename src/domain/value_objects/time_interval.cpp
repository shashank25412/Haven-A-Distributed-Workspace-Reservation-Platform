/**
 * @file time_interval.cpp
 * @brief Implements the reservation time interval domain value object.
 */

#include "haven/domain/value_objects/time_interval.hpp"

#include <stdexcept>

namespace haven::domain {

TimeInterval::TimeInterval(TimePoint start, TimePoint end) : start_(start), end_(end) {
    if (start_ >= end_) {
        throw std::invalid_argument("Time interval start must be earlier than end.");
    }
}

TimeInterval::TimePoint TimeInterval::start() const noexcept {
    return start_;
}

TimeInterval::TimePoint TimeInterval::end() const noexcept {
    return end_;
}

TimeInterval::Duration TimeInterval::duration() const noexcept {
    return end_ - start_;
}

bool TimeInterval::overlaps(const TimeInterval& other) const noexcept {
    return start_ < other.end_ && end_ > other.start_;
}

}  // namespace haven::domain