/**
 * @file metrics_foundation_test.cpp
 * @brief Tests the backend-independent metrics foundation.
 */

#include "haven/application/observability/metrics/metric_label.hpp"
#include "haven/application/observability/metrics/metric_name.hpp"
#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"

#include "application/util/recording_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>

namespace haven::test::application::observability::metrics {
namespace {

using haven::application::observability::metrics::MetricLabel;
using haven::application::observability::metrics::MetricLabels;
using haven::application::observability::metrics::MetricName;
using haven::infrastructure::observability::metrics::NoOpMetricsRecorder;
using namespace std::chrono_literals;

TEST(MetricNameTest, PreservesNonEmptyName) {
    const MetricName name{"haven.requests total"};
    EXPECT_EQ(name.value(), "haven.requests total");
}

TEST(MetricNameTest, RejectsEmptyName) {
    EXPECT_THROW(MetricName{std::string{}}, std::invalid_argument);
}

TEST(MetricNameTest, ComparesByValue) {
    EXPECT_EQ(MetricName{"requests"}, MetricName{"requests"});
    EXPECT_NE(MetricName{"requests"}, MetricName{"failures"});
}

TEST(MetricLabelTest, PreservesNameAndValue) {
    const MetricLabel label{"status", "accepted"};
    EXPECT_EQ(label.name(), "status");
    EXPECT_EQ(label.value(), "accepted");
}

TEST(MetricLabelTest, AllowsEmptyValue) {
    EXPECT_EQ(MetricLabel("status", "").value(), "");
}

TEST(MetricLabelTest, RejectsEmptyName) {
    EXPECT_THROW(MetricLabel("", "accepted"), std::invalid_argument);
}

TEST(MetricLabelTest, ComparesByNameAndValue) {
    EXPECT_EQ(MetricLabel("status", "accepted"), MetricLabel("status", "accepted"));
    EXPECT_NE(MetricLabel("status", "accepted"), MetricLabel("status", "rejected"));
    EXPECT_NE(MetricLabel("status", "accepted"), MetricLabel("result", "accepted"));
}

TEST(NoOpMetricsRecorderTest, AcceptsEveryOperationAndValueWithoutThrowing) {
    NoOpMetricsRecorder recorder;
    const MetricLabels labels{MetricLabel{"status", "accepted"}};

    EXPECT_NO_THROW(recorder.increment_counter(MetricName{"counter"}, 0.0, {}));
    EXPECT_NO_THROW(recorder.increment_counter(MetricName{"counter"}, -2.0, labels));
    EXPECT_NO_THROW(recorder.set_gauge(MetricName{"gauge"}, 3.5, {}));
    EXPECT_NO_THROW(recorder.set_gauge(MetricName{"gauge"}, -3.5, labels));
    EXPECT_NO_THROW(recorder.observe_duration(MetricName{"duration"}, 0us, {}));
    EXPECT_NO_THROW(recorder.observe_duration(MetricName{"duration"}, -5us, labels));
}

TEST(RecordingMetricsRecorderTest, RetainsCallsAndOrderWithinEachCategory) {
    RecordingMetricsRecorder recorder;
    const MetricLabels labels{MetricLabel{"result", "success"}, MetricLabel{"region", ""}};

    recorder.increment_counter(MetricName{"first_counter"}, 1.5, labels);
    recorder.increment_counter(MetricName{"second_counter"}, -2.0, {});
    recorder.set_gauge(MetricName{"first_gauge"}, 0.0, {});
    recorder.set_gauge(MetricName{"second_gauge"}, 9.25, labels);
    recorder.observe_duration(MetricName{"first_duration"}, 17us, labels);
    recorder.observe_duration(MetricName{"second_duration"}, 23us, {});

    ASSERT_EQ(recorder.counter_increments().size(), 2U);
    EXPECT_EQ(recorder.counter_increments()[0],
              (CounterIncrement{MetricName{"first_counter"}, 1.5, labels}));
    EXPECT_EQ(recorder.counter_increments()[1],
              (CounterIncrement{MetricName{"second_counter"}, -2.0, {}}));
    ASSERT_EQ(recorder.gauge_assignments().size(), 2U);
    EXPECT_EQ(recorder.gauge_assignments()[0],
              (GaugeAssignment{MetricName{"first_gauge"}, 0.0, {}}));
    EXPECT_EQ(recorder.gauge_assignments()[1],
              (GaugeAssignment{MetricName{"second_gauge"}, 9.25, labels}));
    ASSERT_EQ(recorder.duration_observations().size(), 2U);
    EXPECT_EQ(recorder.duration_observations()[0],
              (DurationObservation{MetricName{"first_duration"}, 17us, labels}));
    EXPECT_EQ(recorder.duration_observations()[1],
              (DurationObservation{MetricName{"second_duration"}, 23us, {}}));
}

TEST(RecordingMetricsRecorderTest, CopiesTemporaryNamesAndLabels) {
    RecordingMetricsRecorder recorder;
    recorder.increment_counter(MetricName{std::string{"temporary"}},
                               1.0,
                               MetricLabels{MetricLabel{std::string{"key"}, std::string{"value"}}});

    ASSERT_EQ(recorder.counter_increments().size(), 1U);
    EXPECT_EQ(recorder.counter_increments().front().name.value(), "temporary");
    ASSERT_EQ(recorder.counter_increments().front().labels.size(), 1U);
    EXPECT_EQ(recorder.counter_increments().front().labels.front(), MetricLabel("key", "value"));
}

}  // namespace
}  // namespace haven::test::application::observability::metrics
