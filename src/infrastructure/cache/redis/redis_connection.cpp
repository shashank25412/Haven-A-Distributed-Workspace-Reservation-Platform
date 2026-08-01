#include "haven/infrastructure/cache/redis/redis_connection.hpp"

#include <sw/redis++/redis++.h>

namespace haven::infrastructure::cache::redis {

RedisConnection::RedisConnection(const RedisConfiguration& configuration) {
    sw::redis::ConnectionOptions options = sw::redis::Uri{configuration.uri}.connection_options();
    if (!configuration.password.empty()) {
        options.password = configuration.password;
    }
    options.connect_timeout = configuration.connect_timeout;
    options.socket_timeout = configuration.command_timeout;
    sw::redis::ConnectionPoolOptions pool;
    pool.size = 4;
    client_ = std::make_unique<sw::redis::Redis>(options, pool);
}

RedisConnection::~RedisConnection() = default;

sw::redis::Redis& RedisConnection::client() const noexcept {
    return *client_;
}

}  // namespace haven::infrastructure::cache::redis
