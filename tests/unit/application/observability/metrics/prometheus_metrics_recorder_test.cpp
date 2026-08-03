/**
 * @file prometheus_metrics_recorder_test.cpp
 * @brief Tests Haven's Prometheus metrics registry and serializer.
 */

#include "haven/infrastructure/observability/metrics/prometheus_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace haven::test::infrastructure::observability::metrics {
namespace {

using haven::application::observability::metrics::MetricLabel;
using haven::application::observability::metrics::MetricLabels;
using haven::application::observability::metrics::MetricName;
using haven::infrastructure::observability::metrics::PrometheusMetricsRecorder;
using namespace std::chrono_literals;

TEST(PrometheusMetricsRecorderTest, CounterCreatesAccumulatesAndAcceptsZero) {
    PrometheusMetricsRecorder recorder;
    recorder.increment_counter(MetricName{"haven_test_requests_total"}, 0.0, {});
    recorder.increment_counter(MetricName{"haven_test_requests_total"}, 2.5, {});

    EXPECT_EQ(recorder.collect(),
              "# TYPE haven_test_requests_total counter\n"
              "haven_test_requests_total 2.5\n");
}

TEST(PrometheusMetricsRecorderTest, CounterRejectsNegativeIncrement) {
    PrometheusMetricsRecorder recorder;
    EXPECT_THROW(recorder.increment_counter(MetricName{"haven_test_total"}, -1.0, {}),
                 std::invalid_argument);
}

TEST(PrometheusMetricsRecorderTest, CounterCanonicalizesLabelsAndSeparatesDistinctSets) {
    PrometheusMetricsRecorder recorder;
    recorder.increment_counter(MetricName{"haven_test_total"},
                               1.0,
                               {MetricLabel{"zone", "east"}, MetricLabel{"result", "ok"}});
    recorder.increment_counter(MetricName{"haven_test_total"},
                               2.0,
                               {MetricLabel{"result", "ok"}, MetricLabel{"zone", "east"}});
    recorder.increment_counter(
        MetricName{"haven_test_total"}, 4.0, {MetricLabel{"result", "error"}});

    const auto output = recorder.collect();
    EXPECT_NE(output.find("haven_test_total{result=\"ok\",zone=\"east\"} 3"), std::string::npos);
    EXPECT_NE(output.find("haven_test_total{result=\"error\"} 4"), std::string::npos);
}

TEST(PrometheusMetricsRecorderTest, GaugeReplacesValuesAndSupportsSignedValues) {
    PrometheusMetricsRecorder recorder;
    recorder.set_gauge(MetricName{"haven_test_active_items"}, 5.0, {});
    recorder.set_gauge(MetricName{"haven_test_active_items"}, 0.0, {});
    recorder.set_gauge(MetricName{"haven_test_active_items"}, -3.25, {});
    recorder.set_gauge(MetricName{"haven_test_active_items"}, 7.0, {MetricLabel{"zone", "west"}});

    const auto output = recorder.collect();
    EXPECT_NE(output.find("haven_test_active_items -3.25"), std::string::npos);
    EXPECT_NE(output.find("haven_test_active_items{zone=\"west\"} 7"), std::string::npos);
}

TEST(PrometheusMetricsRecorderTest, DurationExportsCountAndExactSecondsSum) {
    PrometheusMetricsRecorder recorder;
    recorder.observe_duration(MetricName{"haven_test_operation_duration_seconds"}, 250000us, {});
    recorder.observe_duration(MetricName{"haven_test_operation_duration_seconds"}, 750000us, {});

    const auto output = recorder.collect();
    EXPECT_NE(output.find("haven_test_operation_duration_seconds_count 2"), std::string::npos);
    EXPECT_NE(output.find("haven_test_operation_duration_seconds_sum 1"), std::string::npos);
}

TEST(PrometheusMetricsRecorderTest, RejectsDuplicateLabelsAndIncompatibleMetricTypes) {
    PrometheusMetricsRecorder recorder;
    EXPECT_THROW(recorder.set_gauge(MetricName{"haven_test_value"},
                                    1.0,
                                    {MetricLabel{"zone", "east"}, MetricLabel{"zone", "west"}}),
                 std::invalid_argument);
    recorder.increment_counter(MetricName{"haven_test_value"}, 1.0, {});
    EXPECT_THROW(recorder.set_gauge(MetricName{"haven_test_value"}, 1.0, {}), std::logic_error);
}

TEST(PrometheusMetricsRecorderTest, RejectsPrometheusIncompatibleNamesAtBackendBoundary) {
    PrometheusMetricsRecorder recorder;
    EXPECT_THROW(recorder.set_gauge(MetricName{"invalid metric"}, 1.0, {}), std::invalid_argument);
    EXPECT_THROW(recorder.set_gauge(
                     MetricName{"valid_metric"}, 1.0, {MetricLabel{"invalid-label", "value"}}),
                 std::invalid_argument);
}

TEST(PrometheusMetricsRecorderTest, EscapesPrometheusLabelValues) {
    PrometheusMetricsRecorder recorder;
    recorder.set_gauge(
        MetricName{"haven_test_value"}, 1.0, {MetricLabel{"detail", "quote\" slash\\ line\nnext"}});

    EXPECT_NE(recorder.collect().find("detail=\"quote\\\" slash\\\\ line\\nnext\""),
              std::string::npos);
}

TEST(PrometheusMetricsRecorderTest, ConcurrentUpdatesRemainConsistent) {
    PrometheusMetricsRecorder recorder;
    constexpr int kThreadCount = 8;
    constexpr int kIncrementsPerThread = 500;
    std::vector<std::thread> threads;
    for (int thread = 0; thread < kThreadCount; ++thread) {
        threads.emplace_back([&recorder]() {
            for (int increment = 0; increment < kIncrementsPerThread; ++increment)
                recorder.increment_counter(MetricName{"haven_test_concurrent_total"}, 1.0, {});
        });
    }
    for (auto& thread : threads)
        thread.join();

    EXPECT_NE(recorder.collect().find("haven_test_concurrent_total 4000"), std::string::npos);
}

}  // namespace
}  // namespace haven::test::infrastructure::observability::metrics
