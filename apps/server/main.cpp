/**
 * @file main.cpp
 * @brief Defines the Haven API process entry point.
 *
 * This file acts as the composition root. It loads validated process
 * configuration, registers presentation routes, and starts the Drogon event
 * loop. Business logic and configuration parsing must remain outside main().
 */

#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/authoritative_resource_query_repository.hpp"
#include "haven/application/resources/cached_resource_query_repository.hpp"
#include "haven/application/resources/get_resource_handler.hpp"
#include "haven/application/resources/resource_query_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/bootstrap/configuration.hpp"
#include "haven/infrastructure/cache/redis/redis_connection.hpp"
#include "haven/infrastructure/cache/redis/redis_resource_detail_cache.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_resource_repository.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/health/live_controller.hpp"
#include "haven/presentation/resources/get_resource_controller.hpp"

#include <drogon/HttpAppFramework.h>

#include <cstdlib>
#include <exception>
#include <memory>

namespace {

[[nodiscard]] haven::logging::LogLevel to_logging_level(
    const haven::bootstrap::LogLevel level) noexcept {
    using BootstrapLogLevel = haven::bootstrap::LogLevel;
    using LoggingLogLevel = haven::logging::LogLevel;

    switch (level) {
        case BootstrapLogLevel::trace:
            return LoggingLogLevel::Trace;
        case BootstrapLogLevel::debug:
            return LoggingLogLevel::Debug;
        case BootstrapLogLevel::info:
            return LoggingLogLevel::Info;
        case BootstrapLogLevel::warn:
            return LoggingLogLevel::Warn;
        case BootstrapLogLevel::error:
            return LoggingLogLevel::Error;
        case BootstrapLogLevel::critical:
            return LoggingLogLevel::Critical;
    }

    return LoggingLogLevel::Info;
}

}  // namespace

int main() {
    try {
        const haven::bootstrap::ApplicationConfiguration configuration =
            haven::bootstrap::load_configuration_from_environment();

        haven::logging::Logger::instance().set_level(to_logging_level(configuration.logging.level));

        namespace couchbase_persistence = haven::infrastructure::persistence::couchbase;

        auto couchbase_connection =
            std::make_shared<couchbase_persistence::CouchbaseConnection>(configuration.couchbase);

        std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository =
            std::make_shared<couchbase_persistence::CouchbaseResourceRepository>(
                couchbase_connection);

        std::shared_ptr<haven::application::reservations::ReservationRepository>
            reservation_repository =
                std::make_shared<couchbase_persistence::CouchbaseReservationRepository>(
                    couchbase_connection);

        HVN_INFO_LOG("Couchbase repositories initialized");

        auto authoritative_query =
            std::make_shared<haven::application::resources::AuthoritativeResourceQueryRepository>(
                *resource_repository);
        std::shared_ptr<haven::application::resources::ResourceQueryRepository> resource_query =
            authoritative_query;
        std::shared_ptr<haven::infrastructure::cache::redis::RedisConnection> redis_connection;
        std::shared_ptr<haven::application::resources::ResourceDetailCache> resource_cache;
        if (configuration.redis.enabled) {
            redis_connection =
                std::make_shared<haven::infrastructure::cache::redis::RedisConnection>(
                    configuration.redis);
            resource_cache =
                std::make_shared<haven::infrastructure::cache::redis::RedisResourceDetailCache>(
                    redis_connection, configuration.redis);
            resource_query =
                std::make_shared<haven::application::resources::CachedResourceQueryRepository>(
                    *resource_cache, *authoritative_query);
            HVN_INFO_LOG("Redis Resource detail cache enabled");
        } else {
            HVN_INFO_LOG("Redis Resource detail cache disabled");
        }

        auto get_resource_handler =
            std::make_shared<haven::application::resources::GetResourceHandler>(*resource_query);

        haven::presentation::health::register_live_route();
        haven::presentation::resources::register_get_resource_route(
            std::move(get_resource_handler));
        HVN_INFO_LOG("HTTP routes registered");

        HVN_INFO_LOG("Starting Haven API on ",
                     configuration.http.address,
                     ':',
                     configuration.http.port,
                     " using ",
                     configuration.http.worker_threads,
                     " HTTP worker thread(s)");

        drogon::app()
            .addListener(configuration.http.address, configuration.http.port)
            .setThreadNum(configuration.http.worker_threads)
            .run();

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        HVN_CRITICAL_LOG("Haven startup failed: ", error.what());
        return EXIT_FAILURE;
    }
}
