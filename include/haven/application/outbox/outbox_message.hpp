/**
 * @file outbox_message.hpp
 * @brief Defines the transport-neutral persisted Outbox message.
 */
#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"

#include <chrono>
#include <cstdint>
#include <string>

namespace haven::application::outbox {

/** @brief Canonical event envelope ready for a future transport adapter. */
struct OutboxMessage {
    haven::domain::EventId event_id;
    haven::domain::OrganizationId organization_id;
    haven::domain::ReservationId aggregate_id;
    std::string aggregate_type;
    std::string event_type;
    std::chrono::system_clock::time_point occurred_at;
    std::uint64_t schema_version;
    std::string serialized_envelope;
    std::uint32_t attempt_count;

    bool operator==(const OutboxMessage&) const = default;
};

}  // namespace haven::application::outbox
