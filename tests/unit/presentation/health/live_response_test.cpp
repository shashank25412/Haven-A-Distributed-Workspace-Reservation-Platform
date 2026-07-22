/**
 * @file live_response_test.cpp
 * @brief Verifies Haven's framework-independent liveness response contract.
 *
 * These tests intentionally avoid starting Drogon or using network resources.
 * They validate only the stable response model returned by the presentation
 * layer's liveness factory.
 */

#include "haven/presentation/health/live_response.hpp"

#include <gtest/gtest.h>

namespace haven::presentation::health {
namespace {

TEST(LiveResponseTest, ReportsHavenApiAsAlive) {
    const LiveResponse response = make_live_response();

    EXPECT_EQ(response.status, "alive");
    EXPECT_EQ(response.service, "haven-api");
}

}  // namespace
}  // namespace haven::presentation::health