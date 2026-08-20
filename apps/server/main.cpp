/**
 * @file main.cpp
 * @brief Defines the Haven API process entry point.
 *
 * This file acts as the composition root. It loads validated process
 * configuration, registers presentation routes, and starts the Drogon event
 * loop. Business logic and configuration parsing must remain outside main().
 */

#include "haven/application/health/readiness.hpp"
#include "haven/application/idempotency/idempotency_repository.hpp"
#include "haven/application/outbox/outbox_publisher.hpp"
#include "haven/application/outbox/system_outbox_publisher_clock.hpp"
#include "haven/application/reservations/approve_reservation_handler.hpp"
#include "haven/application/reservations/create_reservation_handler.hpp"
#include "haven/application/reservations/list_caller_reservations_handler.hpp"
#include "haven/application/reservations/list_decided_approvals_handler.hpp"
#include "haven/application/reservations/list_pending_approvals_handler.hpp"
#include "haven/application/reservations/reject_reservation_handler.hpp"
#include "haven/application/reservations/reservation_repository.hpp"
#include "haven/application/resources/authoritative_resource_query_repository.hpp"
#include "haven/application/resources/cached_resource_query_repository.hpp"
#include "haven/application/resources/get_resource_handler.hpp"
#include "haven/application/resources/search_available_resources_handler.hpp"
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
#include "haven/infrastructure/persistence/couchbase/couchbase_authentication_service.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_idempotency_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_outbox_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_event_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_creation_store.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_reservation_repository.hpp"
#include "haven/infrastructure/persistence/couchbase/couchbase_resource_repository.hpp"
#include "haven/logging/logging.hpp"
#include "haven/presentation/approvals/approve_reservation_controller.hpp"
#include "haven/presentation/approvals/list_pending_approvals_controller.hpp"
#include "haven/presentation/approvals/list_decided_approvals_controller.hpp"
#include "haven/presentation/approvals/reject_reservation_controller.hpp"
#include "haven/presentation/health/live_controller.hpp"
#include "haven/presentation/auth/authentication_controller.hpp"
#include "haven/presentation/health/readiness_controller.hpp"
#include "haven/presentation/observability/metrics/metrics_controller.hpp"
#include "haven/presentation/reservations/create_reservation_controller.hpp"
#include "haven/presentation/reservations/list_my_reservations_controller.hpp"
#include "haven/presentation/resources/get_resource_controller.hpp"
#include "haven/presentation/resources/search_resources_controller.hpp"
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

        auto identity_configuration = configuration.couchbase;
        identity_configuration.bucket_name += "_identity";
        identity_configuration.scope_name = "identity";
        auto identity_connection =
            std::make_shared<couchbase_persistence::CouchbaseConnection>(
                std::move(identity_configuration));
        auto authentication_service =
            std::make_shared<couchbase_persistence::CouchbaseAuthenticationService>(
                identity_connection, "organization-1");

        std::shared_ptr<haven::application::resources::ResourceRepository> resource_repository =
            std::make_shared<couchbase_persistence::CouchbaseResourceRepository>(
                couchbase_connection, *metrics_recorder);

        std::shared_ptr<haven::application::reservations::ReservationRepository>
            reservation_repository =
                std::make_shared<couchbase_persistence::CouchbaseReservationRepository>(
                    couchbase_connection, *metrics_recorder);

        auto idempotency_repository =
            std::make_shared<couchbase_persistence::CouchbaseIdempotencyRepository>(
                couchbase_connection,
                configuration.couchbase.idempotency_retention,
                *metrics_recorder);
        auto reservation_creation_store =
            std::make_shared<couchbase_persistence::CouchbaseReservationCreationStore>(
                couchbase_connection, *metrics_recorder);
        auto reservation_creation_event_store =
            std::make_shared<couchbase_persistence::CouchbaseReservationCreationEventStore>(
                couchbase_connection, *metrics_recorder);

        std::unique_ptr<couchbase_persistence::CouchbaseOutboxRepository> outbox_repository;
        std::unique_ptr<haven::infrastructure::messaging::kafka::KafkaOutboxMessageProducer>
            outbox_producer;
        auto outbox_clock = haven::application::outbox::SystemOutboxPublisherClock{};
        std::unique_ptr<haven::application::outbox::OutboxPublisher> outbox_publisher;
        std::unique_ptr<haven::runtime::outbox::OutboxPublisherWorker> outbox_worker;
        if (configuration.kafka.enabled) {
            outbox_repository = std::make_unique<couchbase_persistence::CouchbaseOutboxRepository>(
                couchbase_connection, *metrics_recorder);
            outbox_producer = std::make_unique<
                haven::infrastructure::messaging::kafka::KafkaOutboxMessageProducer>(
                configuration.kafka, *metrics_recorder);
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
                    redis_connection, configuration.redis, *metrics_recorder);
            resource_query =
                std::make_shared<haven::application::resources::CachedResourceQueryRepository>(
                    *resource_cache, *authoritative_query, *metrics_recorder);
            HVN_INFO_LOG("Redis Resource detail cache enabled");
        } else {
            HVN_INFO_LOG("Redis Resource detail cache disabled");
        }

        auto get_resource_handler =
            std::make_shared<haven::application::resources::GetResourceHandler>(*resource_query);
        auto search_resources_handler =
            std::make_shared<haven::application::resources::SearchAvailableResourcesHandler>(
                *resource_repository, *reservation_repository);
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
        auto list_caller_reservations_handler =
            std::make_shared<haven::application::reservations::ListCallerReservationsHandler>(
                *reservation_repository);
        auto list_pending_approvals_handler =
            std::make_shared<haven::application::reservations::ListPendingApprovalsHandler>(
                *reservation_repository);
        auto list_decided_approvals_handler =
            std::make_shared<haven::application::reservations::ListDecidedApprovalsHandler>(
                *reservation_repository);
        auto approve_reservation_handler =
            std::make_shared<haven::application::reservations::ApproveReservationHandler>(
                *reservation_repository);
        auto reject_reservation_handler =
            std::make_shared<haven::application::reservations::RejectReservationHandler>(
                *reservation_repository);

        namespace app_health = haven::application::health;
        auto couchbase_probe = app_health::FunctionReadinessProbe{
            [couchbase_connection] { return couchbase_connection->is_ready(); }};
        std::unique_ptr<app_health::FunctionReadinessProbe> redis_probe;
        if (redis_connection) {
            redis_probe = std::make_unique<app_health::FunctionReadinessProbe>(
                [redis_connection] { return redis_connection->is_ready(); });
        }
        std::unique_ptr<app_health::FunctionReadinessProbe> kafka_probe;
        std::unique_ptr<app_health::FunctionReadinessProbe> worker_probe;
        if (outbox_producer && outbox_worker) {
            kafka_probe = std::make_unique<app_health::FunctionReadinessProbe>(
                [&outbox_producer] { return outbox_producer->is_ready(); });
            worker_probe = std::make_unique<app_health::FunctionReadinessProbe>(
                [&outbox_worker] { return outbox_worker->is_running(); });
        }
        auto readiness = std::make_shared<app_health::ReadinessService>(
            couchbase_probe, redis_probe.get(), kafka_probe.get(), worker_probe.get());

        haven::presentation::health::register_live_route();
        haven::presentation::auth::register_authentication_routes(authentication_service);
        haven::presentation::health::register_readiness_route(std::move(readiness));
        haven::presentation::resources::register_get_resource_route(
            std::move(get_resource_handler), "organization-1");
        haven::presentation::resources::register_search_resources_route(
            std::move(search_resources_handler), "organization-1");
        haven::presentation::reservations::register_create_reservation_route(
            std::move(create_reservation_handler), authentication_service);
        haven::presentation::reservations::register_list_my_reservations_route(
            std::move(list_caller_reservations_handler), resource_repository, authentication_service);
        haven::presentation::approvals::register_list_pending_approvals_route(
            std::move(list_pending_approvals_handler), resource_repository, authentication_service);
        haven::presentation::approvals::register_list_decided_approvals_route(
            std::move(list_decided_approvals_handler), resource_repository, authentication_service);
        haven::presentation::approvals::register_approve_reservation_route(
            std::move(approve_reservation_handler), authentication_service);
        haven::presentation::approvals::register_reject_reservation_route(
            std::move(reject_reservation_handler), authentication_service);
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
