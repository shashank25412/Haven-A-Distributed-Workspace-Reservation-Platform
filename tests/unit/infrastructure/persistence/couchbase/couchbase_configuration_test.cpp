/**
 * @file couchbase_configuration_test.cpp
 * @brief Tests Couchbase persistence configuration construction.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_configuration.hpp"

#include <gtest/gtest.h>

namespace haven::infrastructure::persistence::couchbase {
namespace {

TEST(CouchbaseConfigurationTest, PreservesSuppliedConfigurationValues) {
    const CouchbaseConfiguration configuration{
        .connection_string = "couchbase://localhost",
        .username = "Admin",
        .password = "Password123",
        .bucket_name = "haven",
        .scope_name = "haven",
    };

    EXPECT_EQ(configuration.connection_string, "couchbase://localhost");
    EXPECT_EQ(configuration.username, "Admin");
    EXPECT_EQ(configuration.password, "Password123");
    EXPECT_EQ(configuration.bucket_name, "haven");
    EXPECT_EQ(configuration.scope_name, "haven");
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase