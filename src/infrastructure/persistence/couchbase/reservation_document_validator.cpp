/**
 * @file reservation_document_validator.cpp
 * @brief Implements reservation document validation and UTC timestamp conversion.
 */

#include "haven/infrastructure/persistence/couchbase/reservation_document_validator.hpp"

#include "haven/domain/value_objects/reservation_kind.hpp"
#include "haven/domain/value_objects/reservation_status.hpp"
#include "haven/logging/logging.hpp"

#include <charconv>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>

namespace haven::infrastructure::persistence::couchbase {
namespace {

using namespace std::chrono;

[[nodiscard]] int parse_component(const std::string_view text,
                                  const std::size_t offset,
                                  const std::size_t length) {
    int value{};
    const char* first = text.data() + offset;
    const char* last = first + length;
    const auto [end, error] = std::from_chars(first, last, value);
    if (error != std::errc{} || end != last) {
        throw std::invalid_argument("Malformed reservation timestamp: " + std::string{text});
    }
    return value;
}

void require_non_empty(const std::string_view value, const std::string_view field_name) {
    if (value.empty()) {
        throw std::invalid_argument("Couchbase reservation document field must not be empty: " +
                                    std::string{field_name});
    }
}

}  // namespace

std::string reservation_timestamp_to_string(const system_clock::time_point timestamp) {
    HVN_TRACE_SCOPE();

    const auto nanosecond_time = time_point_cast<nanoseconds>(timestamp);
    const auto date_days = floor<days>(nanosecond_time);
    const year_month_day date{date_days};
    const hh_mm_ss time{nanosecond_time - date_days};

    char result[31]{};
    const int length = std::snprintf(result,
                                     sizeof(result),
                                     "%04d-%02u-%02uT%02lld:%02lld:%02lld.%09lldZ",
                                     static_cast<int>(date.year()),
                                     static_cast<unsigned>(date.month()),
                                     static_cast<unsigned>(date.day()),
                                     static_cast<long long>(time.hours().count()),
                                     static_cast<long long>(time.minutes().count()),
                                     static_cast<long long>(time.seconds().count()),
                                     static_cast<long long>(time.subseconds().count()));
    if (length != 30) {
        throw std::invalid_argument("Reservation timestamp is outside the supported UTC range");
    }
    return std::string{result, static_cast<std::size_t>(length)};
}

system_clock::time_point reservation_timestamp_from_string(const std::string_view timestamp) {
    HVN_TRACE_SCOPE();

    if (timestamp.size() != 30 || timestamp[4] != '-' || timestamp[7] != '-' ||
        timestamp[10] != 'T' || timestamp[13] != ':' || timestamp[16] != ':' ||
        timestamp[19] != '.' || timestamp[29] != 'Z') {
        throw std::invalid_argument("Malformed reservation timestamp: " + std::string{timestamp});
    }

    const int year_value = parse_component(timestamp, 0, 4);
    const int month_value = parse_component(timestamp, 5, 2);
    const int day_value = parse_component(timestamp, 8, 2);
    const int hour_value = parse_component(timestamp, 11, 2);
    const int minute_value = parse_component(timestamp, 14, 2);
    const int second_value = parse_component(timestamp, 17, 2);
    const int nanosecond_value = parse_component(timestamp, 20, 9);

    const year_month_day date{year{year_value},
                              month{static_cast<unsigned>(month_value)},
                              day{static_cast<unsigned>(day_value)}};
    if (!date.ok() || hour_value > 23 || minute_value > 59 || second_value > 59) {
        throw std::invalid_argument("Malformed reservation timestamp: " + std::string{timestamp});
    }

    const sys_time<nanoseconds> parsed = sys_days{date} + hours{hour_value} +
                                         minutes{minute_value} + seconds{second_value} +
                                         nanoseconds{nanosecond_value};
    return system_clock::time_point{
        duration_cast<system_clock::duration>(parsed.time_since_epoch())};
}

void validate_reservation_document(const ReservationDocument& document) {
    HVN_TRACE_SCOPE();

    if (document.schema_version != kReservationDocumentSchemaVersion) {
        throw std::invalid_argument("Unsupported Couchbase reservation document schema version: " +
                                    std::to_string(document.schema_version));
    }

    require_non_empty(document.reservation_id, "reservationId");
    require_non_empty(document.organization_id, "organizationId");
    require_non_empty(document.resource_id, "resourceId");
    require_non_empty(document.created_by, "createdBy");
    require_non_empty(document.status, "status");
    require_non_empty(document.kind, "kind");
    static_cast<void>(haven::domain::reservation_status_from_string(document.status));
    static_cast<void>(haven::domain::reservation_kind_from_string(document.kind));

    const auto start = reservation_timestamp_from_string(document.start_time);
    const auto end = reservation_timestamp_from_string(document.end_time);
    if (start >= end) {
        throw std::invalid_argument("Reservation document startTime must be before endTime");
    }
    if (document.version == 0) {
        throw std::invalid_argument(
            "Couchbase reservation document version must be greater than zero");
    }
    if (document.approval.has_value()) {
        require_non_empty(document.approval->approved_by, "approval.approvedBy");
        static_cast<void>(reservation_timestamp_from_string(document.approval->approved_at));
    }
    if (document.rejection.has_value()) {
        require_non_empty(document.rejection->rejected_by, "rejection.rejectedBy");
        static_cast<void>(reservation_timestamp_from_string(document.rejection->rejected_at));
    }
    if (document.cancellation.has_value()) {
        require_non_empty(document.cancellation->cancelled_by, "cancellation.cancelledBy");
        static_cast<void>(reservation_timestamp_from_string(document.cancellation->cancelled_at));
    }
}

}  // namespace haven::infrastructure::persistence::couchbase
