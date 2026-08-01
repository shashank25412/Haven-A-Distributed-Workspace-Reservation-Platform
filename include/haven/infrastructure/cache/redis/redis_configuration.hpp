#pragma once

#include <chrono>
#include <string>

namespace haven::infrastructure::cache::redis {

struct RedisConfiguration final {
    bool enabled;
    std::string uri;
    std::string password;
    std::chrono::milliseconds connect_timeout;
    std::chrono::milliseconds command_timeout;
    std::chrono::seconds resource_detail_ttl;
};

}  // namespace haven::infrastructure::cache::redis
