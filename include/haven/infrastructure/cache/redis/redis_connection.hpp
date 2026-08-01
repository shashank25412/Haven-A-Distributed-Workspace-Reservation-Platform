#pragma once

#include "haven/infrastructure/cache/redis/redis_configuration.hpp"

#include <memory>

namespace sw::redis {
class Redis;
}

namespace haven::infrastructure::cache::redis {

class RedisConnection final {
public:
    explicit RedisConnection(const RedisConfiguration& configuration);
    ~RedisConnection();
    [[nodiscard]] sw::redis::Redis& client() const noexcept;

private:
    std::unique_ptr<sw::redis::Redis> client_;
};

}  // namespace haven::infrastructure::cache::redis
