/**
 * @file message_publish_error.hpp
 * @brief Defines transport-neutral message publication failures.
 */
#pragma once

#include <stdexcept>
#include <string>

namespace haven::application::outbox {

enum class MessagePublishErrorCode {
    Unavailable,
    Timeout,
    Authentication,
    Authorization,
    InvalidMessage,
    PublishFailed
};

class MessagePublishError final : public std::runtime_error {
public:
    MessagePublishError(MessagePublishErrorCode code, std::string message);
    [[nodiscard]] MessagePublishErrorCode code() const noexcept;

private:
    MessagePublishErrorCode code_;
};

}  // namespace haven::application::outbox
