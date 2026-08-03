/**
 * @file couchbase_resource_repository_test.cpp
 * @brief Tests independently verifiable Couchbase resource repository behavior.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_resource_repository.hpp"

#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

namespace {

using haven::infrastructure::persistence::couchbase::CouchbaseConnection;
using haven::infrastructure::persistence::couchbase::CouchbaseResourceRepository;

TEST(CouchbaseResourceRepositoryTest, RejectsNullConnection) {
    auto metrics = haven::infrastructure::observability::metrics::NoOpMetricsRecorder{};
    EXPECT_THROW(CouchbaseResourceRepository(std::shared_ptr<CouchbaseConnection>{}, metrics),
                 std::invalid_argument);
}

}  // namespace
