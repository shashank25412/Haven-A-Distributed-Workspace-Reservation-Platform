/** @file kafka_error_mapper_test.cpp @brief Tests Kafka error classification. */
#include "haven/infrastructure/messaging/kafka/kafka_error_mapper.hpp"

#include <gtest/gtest.h>

namespace haven::infrastructure::messaging::kafka {

TEST(KafkaErrorMapperTest, ClassifiesTransportTimeoutAuthenticationAndAuthorization) {
    using enum haven::application::outbox::MessagePublishErrorCode;
    EXPECT_EQ(kafka_publish_error_code(RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN), Unavailable);
    EXPECT_EQ(kafka_publish_error_code(RD_KAFKA_RESP_ERR__TIMED_OUT), Timeout);
    EXPECT_EQ(kafka_publish_error_code(RD_KAFKA_RESP_ERR_SASL_AUTHENTICATION_FAILED),
              Authentication);
    EXPECT_EQ(kafka_publish_error_code(RD_KAFKA_RESP_ERR_TOPIC_AUTHORIZATION_FAILED),
              Authorization);
    EXPECT_EQ(kafka_publish_error_code(RD_KAFKA_RESP_ERR_UNKNOWN), PublishFailed);
}

}  // namespace haven::infrastructure::messaging::kafka
