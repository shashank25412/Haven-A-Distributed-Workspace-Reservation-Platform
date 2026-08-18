#pragma once

#include <chrono>
#include <string_view>

namespace haven::presentation {

[[nodiscard]] std::chrono::system_clock::time_point parse_http_timestamp(
    std::string_view value);

}  // namespace haven::presentation
