/**
 * @file outbox_document.hpp
 * @brief Defines the Couchbase persistence representation of an Outbox event.
 */

#pragma once

#include "haven/domain/value_objects/event_id.hpp"
#include "haven/domain/value_objects/organization_id.hpp"
#include "haven/domain/value_objects/reservation_id.hpp"
#include "haven/infrastructure/persistence/couchbase/outbox_status.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <tao/json/value.hpp>

namespace haven::infrastructure::persistence::couchbase {

inline constexpr std::uint64_t kOutboxDocumentSchemaVersion{1};
inline constexpr const char* kOutboxDocumentType{"outbox"};
inline constexpr const char* kReservationAggregateType{"Reservation"};

inline constexpr const char* kReservationCreatedEventType{"ReservationCreated"};
inline constexpr const char* kReservationConfirmedEventType{"ReservationConfirmed"};
inline constexpr const char* kReservationApprovalRequestedEventType{"ReservationApprovalRequested"};
inline constexpr const char* kReservationRejectedEventType{"ReservationRejected"};
inline constexpr const char* kReservationCancelledEventType{"ReservationCancelled"};
inline constexpr const char* kReservationExtendedEventType{"ReservationExtended"};
inline constexpr const char* kReservationExpiredEventType{"ReservationExpired"};
inline constexpr const char* kReservationCompletedEventType{"ReservationCompleted"};

struct OutboxDocument {
    using TimePoint = std::chrono::system_clock::time_point;

    std::uint64_t schema_version;
    haven::domain::EventId event_id;
    haven::domain::OrganizationId organization_id;
    haven::domain::ReservationId aggregate_id;
    std::string aggregate_type;
    std::string event_type;
    TimePoint occurred_at;
    OutboxStatus status;
    std::uint32_t attempt_count;
    tao::json::value payload;
    std::optional<TimePoint> published_at;

    bool operator==(const OutboxDocument&) const = default;
};

[[nodiscard]] tao::json::value outbox_document_to_json(const OutboxDocument& document);
[[nodiscard]] OutboxDocument outbox_document_from_json(const tao::json::value& json);

}  // namespace haven::infrastructure::persistence::couchbase
