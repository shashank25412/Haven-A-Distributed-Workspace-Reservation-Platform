/** @file message_publish_error.cpp @brief Implements message publication failures. */
#include "haven/application/outbox/message_publish_error.hpp"

#include <utility>

namespace haven::application::outbox {

MessagePublishError::MessagePublishError(MessagePublishErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

MessagePublishErrorCode MessagePublishError::code() const noexcept {
    return code_;
}

}  // namespace haven::application::outbox
