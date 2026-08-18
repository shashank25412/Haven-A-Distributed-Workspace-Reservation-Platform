/**
 * @file create_reservation_request.cpp
 * @brief Implements reservation creation HTTP request parsing.
 */

#include "haven/presentation/reservations/create_reservation_request.hpp"

#include "haven/presentation/http_timestamp.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>

namespace haven::presentation::reservations {
namespace {
using namespace std::chrono;

}  // namespace

CreateReservationRequest CreateReservationRequest::from_json(const Json::Value& json) {
    if (!json.isObject() || !json.isMember("resourceId") || !json["resourceId"].isString() ||
        !json.isMember("startTime") || !json["startTime"].isString() || !json.isMember("endTime") ||
        !json["endTime"].isString() || (json.isMember("purpose") && !json["purpose"].isString()))
        throw std::invalid_argument("Invalid reservation request");
    const auto resource_id = json["resourceId"].asString();
    if (resource_id.empty() || resource_id.size() > 255U ||
        std::any_of(resource_id.begin(), resource_id.end(), [](const unsigned char character) {
            return std::iscntrl(character) != 0 || std::isspace(character) != 0;
        }))
        throw std::invalid_argument("Invalid resource identifier");
    const auto start = haven::presentation::parse_http_timestamp(json["startTime"].asString());
    const auto end = haven::presentation::parse_http_timestamp(json["endTime"].asString());
    return CreateReservationRequest{haven::domain::ResourceId{resource_id},
                                    haven::domain::TimeInterval{start, end},
                                    haven::domain::Purpose{json.get("purpose", "").asString()}};
}

CreateReservationRequest::CreateReservationRequest(haven::domain::ResourceId resource_id,
                                                   haven::domain::TimeInterval interval,
                                                   haven::domain::Purpose purpose)
    : resource_id_(std::move(resource_id)),
      interval_(std::move(interval)),
      purpose_(std::move(purpose)) {}
const haven::domain::ResourceId& CreateReservationRequest::resource_id() const noexcept {
    return resource_id_;
}
const haven::domain::TimeInterval& CreateReservationRequest::interval() const noexcept {
    return interval_;
}
const haven::domain::Purpose& CreateReservationRequest::purpose() const noexcept {
    return purpose_;
}

std::string reservation_http_timestamp(const system_clock::time_point timestamp) {
    const auto value = time_point_cast<nanoseconds>(timestamp);
    const auto date_days = floor<days>(value);
    const year_month_day date{date_days};
    const hh_mm_ss time{value - date_days};
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
        throw std::invalid_argument("Timestamp outside supported range");
    return {result, static_cast<std::size_t>(length)};
}

}  // namespace haven::presentation::reservations
