#include "haven/presentation/reservations/create_reservation_request.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace haven::presentation::reservations {
namespace {
using namespace std::chrono;

int component(const std::string_view text, const std::size_t offset, const std::size_t length) {
    int value{};
    const auto [end, error] =
        std::from_chars(text.data() + offset, text.data() + offset + length, value);
    if (error != std::errc{} || end != text.data() + offset + length)
        throw std::invalid_argument("Malformed timestamp");
    return value;
}

system_clock::time_point parse_timestamp(std::string text) {
    if (text.size() == 20 && text.back() == 'Z')
        text.insert(19, ".000000000");
    if (text.size() != 30 || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
        text[13] != ':' || text[16] != ':' || text[19] != '.' || text[29] != 'Z')
        throw std::invalid_argument("Malformed timestamp");
    const auto date = year_month_day{year{component(text, 0, 4)},
                                     month{static_cast<unsigned>(component(text, 5, 2))},
                                     day{static_cast<unsigned>(component(text, 8, 2))}};
    const auto hour = component(text, 11, 2);
    const auto minute = component(text, 14, 2);
    const auto second = component(text, 17, 2);
    const auto nanos = component(text, 20, 9);
    if (!date.ok() || hour > 23 || minute > 59 || second > 59)
        throw std::invalid_argument("Malformed timestamp");
    const sys_time<nanoseconds> parsed =
        sys_days{date} + hours{hour} + minutes{minute} + seconds{second} + nanoseconds{nanos};
    return system_clock::time_point{
        duration_cast<system_clock::duration>(parsed.time_since_epoch())};
}

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
    const auto start = parse_timestamp(json["startTime"].asString());
    const auto end = parse_timestamp(json["endTime"].asString());
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
