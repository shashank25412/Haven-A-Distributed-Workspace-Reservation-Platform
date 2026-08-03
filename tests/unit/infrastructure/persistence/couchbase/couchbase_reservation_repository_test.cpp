/**
 * @file couchbase_reservation_repository_test.cpp
 * @brief Tests independently observable reservation repository construction behavior.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"

#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

TEST(CouchbaseReservationRepositoryTest, RejectsNullConnection) {
    auto metrics = haven::infrastructure::observability::metrics::NoOpMetricsRecorder{};
    EXPECT_THROW(
        (haven::infrastructure::persistence::couchbase::CouchbaseReservationRepository{
            std::shared_ptr<haven::infrastructure::persistence::couchbase::CouchbaseConnection>{},
            metrics}),
        std::invalid_argument);
}
