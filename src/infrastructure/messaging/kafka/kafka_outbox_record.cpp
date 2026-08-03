/** @file kafka_outbox_record.cpp @brief Implements Outbox-to-Kafka record mapping. */
#include "haven/infrastructure/messaging/kafka/kafka_outbox_record.hpp"

#include "haven/application/outbox/message_publish_error.hpp"

#include <chrono>
#include <cstdio>
#include <string>

namespace haven::infrastructure::messaging::kafka {
namespace {
std::string timestamp(const std::chrono::system_clock::time_point value) {
    using namespace std::chrono;
    const auto nanosecond_time = time_point_cast<nanoseconds>(value);
    const auto date_days = floor<days>(nanosecond_time);
    const year_month_day date{date_days};
    const hh_mm_ss time{nanosecond_time - date_days};
    char result[31]{};
    const auto length = std::snprintf(result,
                                      sizeof(result),
                                      "%04d-%02u-%02uT%02lld:%02lld:%02lld.%09lldZ",
                                      static_cast<int>(date.year()),
                                      static_cast<unsigned>(date.month()),
                                      static_cast<unsigned>(date.day()),
                                      static_cast<long long>(time.hours().count()),
                                      static_cast<long long>(time.minutes().count()),
                                      static_cast<long long>(time.seconds().count()),
                                      static_cast<long long>(time.subseconds().count()));
    if (length != 30)
        throw haven::application::outbox::MessagePublishError{
            haven::application::outbox::MessagePublishErrorCode::InvalidMessage,
            "Outbox occurrence timestamp is outside the supported range"};
    return {result, static_cast<std::size_t>(length)};
}
}  // namespace

KafkaOutboxRecord to_kafka_outbox_record(const haven::application::outbox::OutboxMessage& message,
                                         const KafkaProducerConfiguration& configuration) {
    if (message.serialized_envelope.empty())
        throw haven::application::outbox::MessagePublishError{
            haven::application::outbox::MessagePublishErrorCode::InvalidMessage,
            "Outbox serialized envelope must not be empty"};
    return {.topic = configuration.reservation_events_topic,
            .key = message.organization_id.value() + "::" + message.aggregate_id.value(),
            .payload = message.serialized_envelope,
            .headers = {{"event-id", message.event_id.value()},
                        {"organization-id", message.organization_id.value()},
                        {"aggregate-id", message.aggregate_id.value()},
                        {"aggregate-type", message.aggregate_type},
                        {"event-type", message.event_type},
                        {"schema-version", std::to_string(message.schema_version)},
                        {"occurred-at", timestamp(message.occurred_at)}}};
}

}  // namespace haven::infrastructure::messaging::kafka
