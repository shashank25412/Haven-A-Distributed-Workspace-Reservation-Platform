/** @file readiness_response_test.cpp @brief Tests readiness HTTP response mapping. */
#include "haven/presentation/health/readiness_response.hpp"

#include <gtest/gtest.h>

#include <string>

namespace haven::presentation::health {
namespace {
using application::health::DependencyStatus;
using application::health::ReadinessResult;
using application::health::ReadinessStatus;

TEST(ReadinessResponseTest, ReadyMapsToOkWithAllBoundedComponents) {
    const auto response = ReadinessResponse{ReadinessResult{
        .status = ReadinessStatus::ready,
        .couchbase = DependencyStatus::up,
        .redis = DependencyStatus::disabled,
        .kafka = DependencyStatus::disabled,
        .outbox_publisher = DependencyStatus::disabled,
    }};

    const auto body = response.to_json();

    EXPECT_EQ(response.status_code(), drogon::k200OK);
    EXPECT_EQ(body["status"].asString(), "ready");
    ASSERT_TRUE(body["components"].isObject());
    EXPECT_EQ(body["components"]["couchbase"].asString(), "up");
    EXPECT_EQ(body["components"]["redis"].asString(), "disabled");
    EXPECT_EQ(body["components"]["kafka"].asString(), "disabled");
    EXPECT_EQ(body["components"]["outboxPublisher"].asString(), "disabled");
    EXPECT_EQ(body["components"].size(), 4U);
    EXPECT_EQ(body.size(), 2U);
}

TEST(ReadinessResponseTest, NotReadyMapsToServiceUnavailableWithoutInternalDetails) {
    const auto response = ReadinessResponse{ReadinessResult{
        .status = ReadinessStatus::not_ready,
        .couchbase = DependencyStatus::down,
        .redis = DependencyStatus::up,
        .kafka = DependencyStatus::up,
        .outbox_publisher = DependencyStatus::up,
    }};

    const auto body = response.to_json();
    const auto serialized = body.toStyledString();

    EXPECT_EQ(response.status_code(), drogon::k503ServiceUnavailable);
    EXPECT_EQ(body["status"].asString(), "not_ready");
    EXPECT_EQ(body["components"]["couchbase"].asString(), "down");
    EXPECT_EQ(serialized.find("password"), std::string::npos);
    EXPECT_EQ(serialized.find("localhost"), std::string::npos);
    EXPECT_EQ(serialized.find("exception"), std::string::npos);
    EXPECT_EQ(serialized.find("0x"), std::string::npos);
}

}  // namespace
}  // namespace haven::presentation::health
