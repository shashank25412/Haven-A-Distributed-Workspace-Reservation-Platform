/**
 * @file prometheus_metrics_recorder_test.cpp
 * @brief Tests Haven's Prometheus metrics registry and serializer.
 */

#include "haven/infrastructure/observability/metrics/prometheus_metrics_recorder.hpp"

#include "haven/application/outbox/metrics/outbox_publisher_metrics.hpp"
#include "haven/application/reservations/metrics/reservation_creation_metrics.hpp"
#include "haven/infrastructure/messaging/kafka/metrics/kafka_outbox_producer_metrics.hpp"

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

TEST(PrometheusMetricsRecorderTest, ExposesReservationCreationCatalogMetrics) {
    namespace reservation_metrics = haven::application::reservations::metrics;
    PrometheusMetricsRecorder recorder;
    const MetricLabels labels{reservation_metrics::outcome_label(
        reservation_metrics::ReservationCreationOutcome::created_confirmed)};
    recorder.increment_counter(reservation_metrics::attempts_metric_name(), 1.0, labels);
    recorder.observe_duration(reservation_metrics::duration_metric_name(), 250000us, labels);

    const auto output = recorder.collect();
    EXPECT_NE(output.find("haven_reservation_creation_attempts_total"), std::string::npos);
    EXPECT_NE(output.find("haven_reservation_creation_duration_seconds_count"), std::string::npos);
    EXPECT_NE(output.find("haven_reservation_creation_duration_seconds_sum"), std::string::npos);
    EXPECT_NE(output.find("outcome=\"created_confirmed\""), std::string::npos);
}

TEST(PrometheusMetricsRecorderTest, ExposesOutboxPublisherCatalogMetrics) {
    namespace outbox_metrics = haven::application::outbox::metrics;
    PrometheusMetricsRecorder recorder;
    const MetricLabels cycle_labels{
        outbox_metrics::outcome_label(outbox_metrics::CycleOutcome::completed)};
    const MetricLabels record_labels{
        outbox_metrics::outcome_label(outbox_metrics::RecordOutcome::published_mark_failed)};
    recorder.increment_counter(outbox_metrics::cycles_metric_name(), 1.0, cycle_labels);
    recorder.observe_duration(outbox_metrics::cycle_duration_metric_name(), 100us, cycle_labels);
    recorder.increment_counter(outbox_metrics::records_discovered_metric_name(), 0.0, {});
    recorder.increment_counter(outbox_metrics::record_attempts_metric_name(), 1.0, record_labels);
    recorder.set_gauge(outbox_metrics::worker_running_metric_name(), 1.0, {});

    const auto output = recorder.collect();
    EXPECT_NE(output.find("haven_outbox_publisher_cycles_total"), std::string::npos);
    EXPECT_NE(output.find("haven_outbox_publisher_cycle_duration_seconds_count"),
              std::string::npos);
    EXPECT_NE(output.find("haven_outbox_publisher_records_discovered_total"), std::string::npos);
    EXPECT_NE(output.find("haven_outbox_publisher_record_attempts_total"), std::string::npos);
    EXPECT_NE(output.find("haven_outbox_publisher_worker_running 1"), std::string::npos);
    EXPECT_NE(output.find("outcome=\"published_mark_failed\""), std::string::npos);
}

TEST(PrometheusMetricsRecorderTest, ExposesKafkaOutboxProducerCatalogMetrics) {
    namespace kafka_metrics = haven::infrastructure::messaging::kafka::metrics;
    PrometheusMetricsRecorder recorder;
    const MetricLabels labels{
        kafka_metrics::outcome_label(kafka_metrics::PublishOutcome::acknowledged)};
    recorder.increment_counter(kafka_metrics::attempts_metric_name(), 1.0, labels);
    recorder.observe_duration(kafka_metrics::duration_metric_name(), 250us, labels);
    recorder.increment_counter(kafka_metrics::payload_bytes_metric_name(), 42.0, {});

    const auto output = recorder.collect();
    EXPECT_NE(output.find("haven_kafka_outbox_publish_attempts_total"), std::string::npos);
    EXPECT_NE(output.find("haven_kafka_outbox_publish_duration_seconds_count"), std::string::npos);
    EXPECT_NE(output.find("haven_kafka_outbox_publish_duration_seconds_sum"), std::string::npos);
    EXPECT_NE(output.find("haven_kafka_outbox_payload_bytes_total 42"), std::string::npos);
    EXPECT_NE(output.find("outcome=\"acknowledged\""), std::string::npos);
}

}  // namespace
}  // namespace haven::test::infrastructure::observability::metrics
