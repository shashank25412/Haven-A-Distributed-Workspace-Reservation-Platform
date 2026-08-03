/** @file couchbase_persistence_metrics_test.cpp @brief Tests Couchbase operation metrics. */
#include "haven/infrastructure/persistence/couchbase/metrics/couchbase_persistence_metrics.hpp"

#include "application/util/recording_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace haven::infrastructure::persistence::couchbase::metrics {
namespace {
using RecordingMetrics = haven::test::application::observability::metrics::RecordingMetricsRecorder;
using haven::application::RepositoryError;
using haven::application::RepositoryErrorCode;

class ThrowingMetrics final : public application::observability::metrics::MetricsRecorder {
public:
    void increment_counter(const application::observability::metrics::MetricName&,
                           double,
                           const application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
    void set_gauge(const application::observability::metrics::MetricName&,
                   double,
                   const application::observability::metrics::MetricLabels&) override {}
    void observe_duration(const application::observability::metrics::MetricName&,
                          std::chrono::microseconds,
                          const application::observability::metrics::MetricLabels&) override {
        throw std::runtime_error{"metrics failure"};
    }
};
}  // namespace

TEST(CouchbasePersistenceMetricsTest, RecordsOneSuccessfulOperationAndDuration) {
    auto recorder = RecordingMetrics{};
    const auto metrics = OperationMetrics{recorder};

    EXPECT_EQ(metrics.record(Repository::resource, Operation::find_by_id, [] { return 7; }), 7);

    ASSERT_EQ(recorder.counter_increments().size(), 1U);
    EXPECT_EQ(recorder.counter_increments()[0].name.value(),
              "haven_couchbase_repository_operations_total");
    EXPECT_EQ(recorder.counter_increments()[0].labels[0].value(), "resource");
    EXPECT_EQ(recorder.counter_increments()[0].labels[1].value(), "find_by_id");
    EXPECT_EQ(recorder.counter_increments()[0].labels[2].value(), "success");
    ASSERT_EQ(recorder.duration_observations().size(), 1U);
    EXPECT_EQ(recorder.duration_observations()[0].labels, recorder.counter_increments()[0].labels);
}

TEST(CouchbasePersistenceMetricsTest, RecordsCasConflictAndPreservesException) {
    auto recorder = RecordingMetrics{};
    const auto metrics = OperationMetrics{recorder};

    EXPECT_THROW(
        metrics.record(
            Repository::reservation,
            Operation::update,
            [] { throw RepositoryError{RepositoryErrorCode::ConcurrencyConflict, "stale"}; }),
        RepositoryError);

    ASSERT_EQ(recorder.counter_increments().size(), 2U);
    EXPECT_EQ(recorder.counter_increments()[0].labels[2].value(), "concurrency_conflict");
    EXPECT_EQ(recorder.counter_increments()[1].name.value(),
              "haven_couchbase_concurrency_conflicts_total");
}

TEST(CouchbasePersistenceMetricsTest, RecordsTopLevelTransactionOnlyOnce) {
    auto recorder = RecordingMetrics{};
    const auto metrics = OperationMetrics{recorder};

    EXPECT_EQ(metrics.record_transaction([] { return 9; }), 9);

    ASSERT_EQ(recorder.counter_increments().size(), 2U);
    EXPECT_EQ(recorder.counter_increments()[0].labels[2].value(), "success");
    EXPECT_EQ(recorder.counter_increments()[1].name.value(),
              "haven_couchbase_reservation_creation_transactions_total");
    EXPECT_EQ(recorder.counter_increments()[1].labels[0].value(), "committed");
    EXPECT_EQ(recorder.duration_observations().size(), 2U);
}

TEST(CouchbasePersistenceMetricsTest, MetricsFailuresPreserveResultsAndRepositoryErrors) {
    auto recorder = ThrowingMetrics{};
    const auto metrics = OperationMetrics{recorder};
    EXPECT_EQ(metrics.record(Repository::resource, Operation::find_by_id, [] { return 11; }), 11);
    EXPECT_THROW(
        metrics.record(
            Repository::reservation,
            Operation::update,
            [] { throw RepositoryError{RepositoryErrorCode::ConcurrencyConflict, "stale"}; }),
        RepositoryError);
}

}  // namespace haven::infrastructure::persistence::couchbase::metrics
