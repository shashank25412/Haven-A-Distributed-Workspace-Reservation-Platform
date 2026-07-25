/**
 * @file time_interval.hpp
 * @brief Defines the reservation time interval domain value object.
 */

#pragma once

#include <chrono>
#include <compare>

namespace haven::domain {

/**
 * @brief Represents a valid half-open time interval using [start, end) semantics.
 *
 * A time interval requires its start time to be strictly earlier than its end
 * time. Duration limits are evaluated separately because they depend on the
 * reservation kind and caller authorization.
 */
class TimeInterval final {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using Duration = std::chrono::system_clock::duration;

    /**
     * @brief Constructs a valid half-open time interval.
     *
     * @param start Inclusive interval start time.
     * @param end Exclusive interval end time.
     *
     * @throws std::invalid_argument when start is not earlier than end.
     */
    TimeInterval(TimePoint start, TimePoint end);

    /**
     * @brief Returns the inclusive start of the interval.
     *
     * @return Interval start time.
     */
    [[nodiscard]] TimePoint start() const noexcept;

    /**
     * @brief Returns the exclusive end of the interval.
     *
     * @return Interval end time.
     */
    [[nodiscard]] TimePoint end() const noexcept;

    /**
     * @brief Returns the duration between the start and end times.
     *
     * @return Interval duration.
     */
    [[nodiscard]] Duration duration() const noexcept;

    /**
     * @brief Determines whether this interval overlaps another interval.
     *
     * Adjacent intervals do not overlap because interval ends are exclusive.
     *
     * @param other Interval to compare against.
     *
     * @return true when the intervals overlap; otherwise, false.
     */
    [[nodiscard]] bool overlaps(const TimeInterval& other) const noexcept;

    auto operator<=>(const TimeInterval&) const = default;

private:
    TimePoint start_;
    TimePoint end_;
};

}  // namespace haven::domain