/** @file kafka_error_mapper.cpp @brief Implements librdkafka error classification. */
#include "haven/infrastructure/messaging/kafka/kafka_error_mapper.hpp"

namespace haven::infrastructure::messaging::kafka {

haven::application::outbox::MessagePublishErrorCode kafka_publish_error_code(
    const rd_kafka_resp_err_t error) noexcept {
    using enum haven::application::outbox::MessagePublishErrorCode;
    switch (error) {
        case RD_KAFKA_RESP_ERR__TIMED_OUT:
        case RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:
        case RD_KAFKA_RESP_ERR_REQUEST_TIMED_OUT:
            return Timeout;
        case RD_KAFKA_RESP_ERR__TRANSPORT:
        case RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN:
        case RD_KAFKA_RESP_ERR_BROKER_NOT_AVAILABLE:
            return Unavailable;
        case RD_KAFKA_RESP_ERR_SASL_AUTHENTICATION_FAILED:
            return Authentication;
        case RD_KAFKA_RESP_ERR_TOPIC_AUTHORIZATION_FAILED:
        case RD_KAFKA_RESP_ERR_GROUP_AUTHORIZATION_FAILED:
        case RD_KAFKA_RESP_ERR_CLUSTER_AUTHORIZATION_FAILED:
            return Authorization;
        case RD_KAFKA_RESP_ERR__INVALID_ARG:
        case RD_KAFKA_RESP_ERR_INVALID_MSG:
            return InvalidMessage;
        default:
            return PublishFailed;
    }
}

}  // namespace haven::infrastructure::messaging::kafka
