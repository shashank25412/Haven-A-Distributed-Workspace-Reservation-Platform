/** @file kafka_error_mapper.hpp @brief Declares librdkafka error classification. */
#pragma once

#include "haven/application/outbox/message_publish_error.hpp"

#include <librdkafka/rdkafka.h>

namespace haven::infrastructure::messaging::kafka {

[[nodiscard]] haven::application::outbox::MessagePublishErrorCode kafka_publish_error_code(
    rd_kafka_resp_err_t error) noexcept;

}  // namespace haven::infrastructure::messaging::kafka
