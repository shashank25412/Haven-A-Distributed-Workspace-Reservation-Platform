/**
 * @file live_response_test.cpp
 * @brief Tests Haven's liveness response serialization.
 */

#include "haven/presentation/health/live_response.hpp"

#include <gtest/gtest.h>

namespace haven::presentation::health {
namespace {

TEST(LiveResponseTest, ToJson_ShouldReturnUpStatus) {
    const LiveResponse live_response;

    const Json::Value response = live_response.to_json();

    ASSERT_TRUE(response.isObject());
    ASSERT_TRUE(response.isMember("status"));

    EXPECT_EQ(response["status"].asString(), "UP");
    EXPECT_EQ(response.size(), 1U);
}

}  // namespace
}  // namespace haven::presentation::health