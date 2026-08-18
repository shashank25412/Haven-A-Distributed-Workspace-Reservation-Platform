#include "haven/presentation/http_timestamp.hpp"

#include <charconv>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace haven::presentation {
namespace {
using namespace std::chrono;

int component(const std::string_view text, const std::size_t offset, const std::size_t length) {
    int value{};
    const auto [end, error] =
        std::from_chars(text.data() + offset, text.data() + offset + length, value);
    if (error != std::errc{} || end != text.data() + offset + length) {
        throw std::invalid_argument("Malformed timestamp");
    }
    return value;
}

}  // namespace

system_clock::time_point parse_http_timestamp(const std::string_view value) {
    auto text = std::string{value};
    if (text.size() == 20U && text.back() == 'Z') text.insert(19U, ".000000000");
    if (text.size() != 30U || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
        text[13] != ':' || text[16] != ':' || text[19] != '.' || text[29] != 'Z') {
        throw std::invalid_argument("Malformed timestamp");
    }
    const auto date = year_month_day{year{component(text, 0U, 4U)},
                                     month{static_cast<unsigned>(component(text, 5U, 2U))},
                                     day{static_cast<unsigned>(component(text, 8U, 2U))}};
    const auto hour = component(text, 11U, 2U);
    const auto minute = component(text, 14U, 2U);
    const auto second = component(text, 17U, 2U);
    const auto nanos = component(text, 20U, 9U);
    if (!date.ok() || hour > 23 || minute > 59 || second > 59) {
        throw std::invalid_argument("Malformed timestamp");
    }
    const sys_time<nanoseconds> parsed =
        sys_days{date} + hours{hour} + minutes{minute} + seconds{second} + nanoseconds{nanos};
    return system_clock::time_point{
        duration_cast<system_clock::duration>(parsed.time_since_epoch())};
}

}  // namespace haven::presentation
