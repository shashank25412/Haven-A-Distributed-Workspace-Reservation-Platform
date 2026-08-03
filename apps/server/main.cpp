/**
 * @file main.cpp
 * @brief Defines the Haven API process entry point.
 *
 * This file acts as the composition root. It loads validated process
 * configuration, registers presentation routes, and starts the Drogon event
 * loop. Business logic and configuration parsing must remain outside main().
 */

#include "haven/application/idempotency/idempotency_repository.hpp"
#include "haven/application/outbox/outbox_publisher.hpp"
#include "haven/application/outbox/system_outbox_publisher_clock.hpp"
#include "haven/application/reservations/create_reservation_handler.hpp"
#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/authoritative_resource_query_repository.hpp"
#include "haven/application/resources/cached_resource_query_repository.hpp"
#include "haven/application/resources/get_resource_handler.hpp"
#include "haven/application/resources/resource_query_repository.hpp"
#include "haven/application/resources/resource_repository.hpp"
#include "haven/bootstrap/configuration.hpp"
#include "haven/domain/policies/reservation_creation_policy.hpp"
#include "haven/infrastructure/cache/redis/redis_connection.hpp"
#include "haven/infrastructure/cache/redis/redis_resource_detail_cache.hpp"
#include "haven/infrastructure/messaging/kafka/kafka_outbox_message_producer.hpp"
#include "haven/infrastructure/observability/metrics/no_op_metrics_recorder.hpp"
#include "haven/infrastructure/observability/metrics/prometheus_metrics_recorder.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_connection.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_idempotency_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_outbox_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_event_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_resource_repository.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/health/live_controller.hpp"
#include "haven/presentation/observability/metrics/metrics_controller.hpp"
#include "haven/presentation/reservations/create_reservation_controller.hpp"
#include "haven/presentation/resources/get_resource_controller.hpp"
#include "haven/runtime/outbox/outbox_publisher_worker.hpp"

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

        std::shared_ptr<haven::application::observability::metrics::MetricsRecorder>
            metrics_recorder;
        if (configuration.metrics.enabled) {
            auto prometheus_recorder = std::make_shared<
                haven::infrastructure::observability::metrics::PrometheusMetricsRecorder>();
            metrics_recorder = prometheus_recorder;
            haven::presentation::observability::metrics::register_metrics_route(
                std::move(prometheus_recorder));
            HVN_INFO_LOG("Prometheus metrics enabled at GET /metrics");
        } else {
            metrics_recorder = std::make_shared<
                haven::infrastructure::observability::metrics::NoOpMetricsRecorder>();
            HVN_INFO_LOG("Metrics disabled; GET /metrics is not registered");
        }
        static_cast<void>(metrics_recorder);

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

        auto idempotency_repository =
            std::make_shared<couchbase_persistence::CouchbaseIdempotencyRepository>(
                couchbase_connection, configuration.couchbase.idempotency_retention);
        auto reservation_creation_store =
            std::make_shared<couchbase_persistence::CouchbaseReservationCreationStore>(
                couchbase_connection);
        auto reservation_creation_event_store =
            std::make_shared<couchbase_persistence::CouchbaseReservationCreationEventStore>(
                couchbase_connection);

        std::unique_ptr<couchbase_persistence::CouchbaseOutboxRepository> outbox_repository;
        std::unique_ptr<haven::infrastructure::messaging::kafka::KafkaOutboxMessageProducer>
            outbox_producer;
        auto outbox_clock = haven::application::outbox::SystemOutboxPublisherClock{};
        std::unique_ptr<haven::application::outbox::OutboxPublisher> outbox_publisher;
        std::unique_ptr<haven::runtime::outbox::OutboxPublisherWorker> outbox_worker;
        if (configuration.kafka.enabled) {
            outbox_repository = std::make_unique<couchbase_persistence::CouchbaseOutboxRepository>(
                couchbase_connection);
            outbox_producer = std::make_unique<
                haven::infrastructure::messaging::kafka::KafkaOutboxMessageProducer>(
                configuration.kafka);
            outbox_publisher = std::make_unique<haven::application::outbox::OutboxPublisher>(
                *outbox_repository, *outbox_producer, outbox_clock, *metrics_recorder);
            outbox_worker = std::make_unique<haven::runtime::outbox::OutboxPublisherWorker>(
                *outbox_publisher,
                configuration.outbox_publisher.batch_size,
                configuration.outbox_publisher.poll_interval,
                *metrics_recorder);
            HVN_INFO_LOG("Kafka Outbox publishing enabled");
        } else {
            HVN_INFO_LOG("Kafka Outbox publishing disabled; Pending records remain durable");
        }

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
        auto reservation_creation_policy =
            std::make_shared<haven::domain::ReservationCreationPolicy>();
        auto create_reservation_handler =
            std::make_shared<haven::application::reservations::CreateReservationHandler>(
                *resource_repository,
                *reservation_repository,
                *reservation_creation_store,
                *reservation_creation_event_store,
                *idempotency_repository,
                *reservation_creation_policy,
                *metrics_recorder);

        haven::presentation::health::register_live_route();
        haven::presentation::resources::register_get_resource_route(
            std::move(get_resource_handler));
        haven::presentation::reservations::register_create_reservation_route(
            std::move(create_reservation_handler));
        HVN_INFO_LOG("HTTP routes registered");

        HVN_INFO_LOG("Starting Haven API on ",
                     configuration.http.address,
                     ':',
                     configuration.http.port,
                     " using ",
                     configuration.http.worker_threads,
                     " HTTP worker thread(s)");

        if (outbox_worker)
            outbox_worker->start();

        drogon::app()
            .addListener(configuration.http.address, configuration.http.port)
            .setThreadNum(configuration.http.worker_threads)
            .run();

        if (outbox_worker)
            outbox_worker->stop();

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        HVN_CRITICAL_LOG("Haven startup failed: ", error.what());
        return EXIT_FAILURE;
    }
}
