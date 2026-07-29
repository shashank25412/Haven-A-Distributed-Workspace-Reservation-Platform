/**
 * @file couchbase_connection_test.cpp
 * @brief Tests Couchbase connection configuration validation.
 */

#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <utility>

namespace haven::infrastructure::persistence::couchbase {
namespace {

class CouchbaseConnectionTest : public testing::Test {
protected:
    [[nodiscard]] static CouchbaseConfiguration valid_configuration() {
        return CouchbaseConfiguration{
            .connection_string = "couchbase://localhost",
            .username = "Administrator",
            .password = "Password123!",
            .bucket_name = "haven",
            .scope_name = "haven",
        };
    }
};

TEST_F(CouchbaseConnectionTest, RejectsEmptyConnectionString) {
    auto configuration = valid_configuration();
    configuration.connection_string.clear();

    EXPECT_THROW(CouchbaseConnection connection{std::move(configuration)}, std::invalid_argument);
}

TEST_F(CouchbaseConnectionTest, RejectsEmptyUsername) {
    auto configuration = valid_configuration();
    configuration.username.clear();

    EXPECT_THROW(CouchbaseConnection connection{std::move(configuration)}, std::invalid_argument);
}

TEST_F(CouchbaseConnectionTest, RejectsEmptyPassword) {
    auto configuration = valid_configuration();
    configuration.password.clear();

    EXPECT_THROW(CouchbaseConnection connection{std::move(configuration)}, std::invalid_argument);
}

TEST_F(CouchbaseConnectionTest, RejectsEmptyBucketName) {
    auto configuration = valid_configuration();
    configuration.bucket_name.clear();

    EXPECT_THROW(CouchbaseConnection connection{std::move(configuration)}, std::invalid_argument);
}

TEST_F(CouchbaseConnectionTest, RejectsEmptyScopeName) {
    auto configuration = valid_configuration();
    configuration.scope_name.clear();

    EXPECT_THROW(CouchbaseConnection connection{std::move(configuration)}, std::invalid_argument);
}

}  // namespace
}  // namespace haven::infrastructure::persistence::couchbase