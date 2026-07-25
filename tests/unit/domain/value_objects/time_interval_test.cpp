/**
 * @file time_interval_test.cpp
 * @brief Tests the reservation time interval domain value object.
 */

#include "haven/domain/value_objects/time_interval.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace haven::domain {
namespace {

using namespace std::chrono_literals;

TEST(TimeIntervalTest, Constructor_ShouldStoreBoundaries_WhenIntervalIsValid) {
    const TimeInterval::TimePoint start{};
    const TimeInterval::TimePoint end = start + 1h;
    const TimeInterval interval{start, end};

    EXPECT_EQ(interval.start(), start);
    EXPECT_EQ(interval.end(), end);
}

TEST(TimeIntervalTest, Constructor_ShouldThrow_WhenStartEqualsEnd) {
    const TimeInterval::TimePoint time{};

    EXPECT_THROW((TimeInterval{time, time}), std::invalid_argument);
}

TEST(TimeIntervalTest, Constructor_ShouldThrow_WhenStartIsAfterEnd) {
    const TimeInterval::TimePoint start = TimeInterval::TimePoint{} + 2h;
    const TimeInterval::TimePoint end = TimeInterval::TimePoint{} + 1h;

    EXPECT_THROW((TimeInterval{start, end}), std::invalid_argument);
}

TEST(TimeIntervalTest, Duration_ShouldReturnDifferenceBetweenEndAndStart) {
    const TimeInterval::TimePoint start{};
    const TimeInterval interval{start, start + 90min};

    EXPECT_EQ(interval.duration(), 90min);
}

TEST(TimeIntervalTest, Overlaps_ShouldReturnTrue_WhenIntervalsPartiallyOverlap) {
    const TimeInterval::TimePoint base{};
    const TimeInterval first{base, base + 2h};
    const TimeInterval second{base + 1h, base + 3h};

    EXPECT_TRUE(first.overlaps(second));
    EXPECT_TRUE(second.overlaps(first));
}

TEST(TimeIntervalTest, Overlaps_ShouldReturnTrue_WhenOneIntervalContainsAnother) {
    const TimeInterval::TimePoint base{};
    const TimeInterval outer{base, base + 4h};
    const TimeInterval inner{base + 1h, base + 2h};

    EXPECT_TRUE(outer.overlaps(inner));
    EXPECT_TRUE(inner.overlaps(outer));
}

TEST(TimeIntervalTest, Overlaps_ShouldReturnFalse_WhenIntervalsAreAdjacent) {
    const TimeInterval::TimePoint base{};
    const TimeInterval first{base, base + 1h};
    const TimeInterval second{base + 1h, base + 2h};

    EXPECT_FALSE(first.overlaps(second));
    EXPECT_FALSE(second.overlaps(first));
}

TEST(TimeIntervalTest, Overlaps_ShouldReturnFalse_WhenIntervalsAreSeparated) {
    const TimeInterval::TimePoint base{};
    const TimeInterval first{base, base + 1h};
    const TimeInterval second{base + 2h, base + 3h};

    EXPECT_FALSE(first.overlaps(second));
    EXPECT_FALSE(second.overlaps(first));
}

}  // namespace
}  // namespace haven::domain