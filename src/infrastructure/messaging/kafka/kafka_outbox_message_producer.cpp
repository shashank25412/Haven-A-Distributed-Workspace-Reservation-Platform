/**
 * @file kafka_outbox_message_producer.cpp
 * @brief Implements synchronous broker-acknowledged Kafka publication.
 */
#include "haven/infrastructure/messaging/kafka/kafka_outbox_message_producer.hpp"

#include "haven/application/outbox/message_publish_error.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_error_mapper.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_outbox_record.hpp"
#include "haven/logging/logging.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace haven::infrastructure::messaging::kafka {
namespace {
using haven::application::outbox::MessagePublishError;
using haven::application::outbox::MessagePublishErrorCode;

struct DeliveryState {
    bool completed{};
    rd_kafka_resp_err_t error{RD_KAFKA_RESP_ERR_NO_ERROR};
};

void delivery_report(rd_kafka_t*, const rd_kafka_message_t* message, void*) {
    auto* state = static_cast<DeliveryState*>(message->_private);
    state->error = message->err;
    state->completed = true;
}

void set_configuration(rd_kafka_conf_t* configuration, const char* name, const std::string& value) {
    char error[512]{};
    if (rd_kafka_conf_set(configuration, name, value.c_str(), error, sizeof(error)) !=
        RD_KAFKA_CONF_OK)
        throw MessagePublishError{MessagePublishErrorCode::InvalidMessage,
                                  std::string{"Invalid Kafka producer configuration: "} + error};
}

std::unique_ptr<rd_kafka_t, KafkaOutboxMessageProducer::ProducerDeleter> make_producer(
    const KafkaProducerConfiguration& settings) {
    auto* configuration = rd_kafka_conf_new();
    try {
        set_configuration(configuration, "bootstrap.servers", settings.brokers);
        set_configuration(configuration, "client.id", settings.client_id);
        set_configuration(configuration, "acks", "all");
        set_configuration(
            configuration, "message.timeout.ms", std::to_string(settings.delivery_timeout.count()));
        rd_kafka_conf_set_dr_msg_cb(configuration, delivery_report);
        char error[512]{};
        auto* producer = rd_kafka_new(RD_KAFKA_PRODUCER, configuration, error, sizeof(error));
        if (!producer)
            throw MessagePublishError{MessagePublishErrorCode::InvalidMessage,
                                      std::string{"Unable to create Kafka producer: "} + error};
        return std::unique_ptr<rd_kafka_t, KafkaOutboxMessageProducer::ProducerDeleter>{producer};
    } catch (...) {
        rd_kafka_conf_destroy(configuration);
        throw;
    }
}
}  // namespace

void KafkaOutboxMessageProducer::ProducerDeleter::operator()(rd_kafka_t* producer) const noexcept {
    rd_kafka_destroy(producer);
}

KafkaOutboxMessageProducer::KafkaOutboxMessageProducer(KafkaProducerConfiguration configuration)
    : configuration_(std::move(configuration)), producer_(make_producer(configuration_)) {
    HVN_DEBUG_LOG("Kafka producer initialized for topic ", configuration_.reservation_events_topic);
}

KafkaOutboxMessageProducer::~KafkaOutboxMessageProducer() {
    const auto lock = std::scoped_lock{mutex_};
    if (producer_) {
        static_cast<void>(rd_kafka_flush(
            producer_.get(), static_cast<int>(configuration_.acknowledgement_timeout.count())));
        producer_.reset();
    }
    timed_out_delivery_states_.clear();
}

void KafkaOutboxMessageProducer::publish(const haven::application::outbox::OutboxMessage& message) {
    HVN_TRACE_SCOPE();
    const auto record = to_kafka_outbox_record(message, configuration_);
    const auto lock = std::scoped_lock{mutex_};
    auto* headers = rd_kafka_headers_new(record.headers.size());
    for (const auto& [name, value] : record.headers) {
        if (rd_kafka_header_add(headers,
                                name.c_str(),
                                -1,
                                value.data(),
                                static_cast<std::ptrdiff_t>(value.size())) !=
            RD_KAFKA_RESP_ERR_NO_ERROR) {
            rd_kafka_headers_destroy(headers);
            throw MessagePublishError{MessagePublishErrorCode::InvalidMessage,
                                      "Unable to construct Kafka record headers"};
        }
    }
    auto state = std::make_shared<DeliveryState>();
    const auto error = rd_kafka_producev(
        producer_.get(),
        RD_KAFKA_V_TOPIC(record.topic.c_str()),
        RD_KAFKA_V_KEY(record.key.data(), record.key.size()),
        RD_KAFKA_V_VALUE(const_cast<char*>(record.payload.data()), record.payload.size()),
        RD_KAFKA_V_HEADERS(headers),
        RD_KAFKA_V_OPAQUE(state.get()),
        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
        RD_KAFKA_V_END);
    if (error != RD_KAFKA_RESP_ERR_NO_ERROR) {
        rd_kafka_headers_destroy(headers);
        throw MessagePublishError{kafka_publish_error_code(error),
                                  std::string{"Kafka enqueue failed: "} + rd_kafka_err2str(error)};
    }
    HVN_DEBUG_LOG("Awaiting Kafka acknowledgement for event ", message.event_id.value());
    const auto deadline = std::chrono::steady_clock::now() + configuration_.acknowledgement_timeout;
    while (!state->completed && std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        const auto poll_milliseconds =
            std::clamp<long long>(static_cast<long long>(remaining.count()), 1, 50);
        static_cast<void>(rd_kafka_poll(producer_.get(), static_cast<int>(poll_milliseconds)));
    }
    if (!state->completed) {
        HVN_WARN_LOG("Kafka acknowledgement timed out for event ", message.event_id.value());
        timed_out_delivery_states_.push_back(state);
        throw MessagePublishError{MessagePublishErrorCode::Timeout,
                                  "Kafka acknowledgement timed out; delivery is ambiguous"};
    }
    if (state->error != RD_KAFKA_RESP_ERR_NO_ERROR) {
        HVN_ERROR_LOG("Kafka publication failed for event ", message.event_id.value());
        throw MessagePublishError{
            kafka_publish_error_code(state->error),
            std::string{"Kafka delivery failed: "} + rd_kafka_err2str(state->error)};
    }
}

}  // namespace haven::infrastructure::messaging::kafka
