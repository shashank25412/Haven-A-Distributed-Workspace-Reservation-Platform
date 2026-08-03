# Haven infrastructure module build configuration.
#
# This file defines adapters for external systems such as persistence,
# caching, messaging, and notifications. Infrastructure targets implement
# application-owned ports while keeping external SDK dependencies isolated
# from the domain and application layers.

find_package(
    couchbase_cxx_client
    CONFIG
    REQUIRED
)
find_package(hiredis CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(RdKafka CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)

add_library(haven_infrastructure_metrics
    observability/metrics/no_op_metrics_recorder.cpp
    observability/metrics/prometheus_metrics_recorder.cpp
)
add_library(haven::infrastructure::metrics ALIAS haven_infrastructure_metrics)
target_include_directories(haven_infrastructure_metrics PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(haven_infrastructure_metrics PUBLIC cxx_std_20)
target_link_libraries(haven_infrastructure_metrics
    PUBLIC haven::application
    PRIVATE prometheus-cpp::core
)
haven_apply_project_options(haven_infrastructure_metrics)

add_library(haven_infrastructure_kafka
    messaging/kafka/kafka_outbox_record.cpp
    messaging/kafka/kafka_error_mapper.cpp
    messaging/kafka/kafka_outbox_message_producer.cpp
    messaging/kafka/metrics/kafka_outbox_producer_metrics.cpp
)
add_library(haven::infrastructure::kafka ALIAS haven_infrastructure_kafka)
target_include_directories(haven_infrastructure_kafka PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(haven_infrastructure_kafka PUBLIC cxx_std_20)
target_link_libraries(haven_infrastructure_kafka
    PUBLIC haven_application haven_domain RdKafka::rdkafka
    PRIVATE haven_logging
)
haven_apply_project_options(haven_infrastructure_kafka)

add_library(haven_infrastructure_redis
    cache/redis/redis_connection.cpp
    cache/redis/resource_detail_cache_key.cpp
    cache/redis/cached_resource_record.cpp
    cache/redis/redis_resource_detail_cache.cpp
    cache/redis/metrics/redis_resource_cache_metrics.cpp
)
add_library(haven::infrastructure::redis ALIAS haven_infrastructure_redis)
target_include_directories(haven_infrastructure_redis PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(haven_infrastructure_redis PUBLIC cxx_std_20)
target_link_libraries(haven_infrastructure_redis
    PUBLIC haven_application haven_domain
    PRIVATE haven_logging redis++::redis++_static JsonCpp::JsonCpp
)
haven_apply_project_options(haven_infrastructure_redis)

add_library(
    haven_infrastructure_couchbase

    persistence/couchbase/couchbase_connection.cpp
    persistence/couchbase/couchbase_document_key.cpp
    persistence/couchbase/idempotency_document_key.cpp
    persistence/couchbase/idempotency_document.cpp
    persistence/couchbase/idempotency_document_mapper.cpp
    persistence/couchbase/idempotency_document_validator.cpp
    persistence/couchbase/outbox_status.cpp
    persistence/couchbase/outbox_document.cpp
    persistence/couchbase/outbox_document_mapper.cpp
    persistence/couchbase/outbox_document_validator.cpp
    persistence/couchbase/outbox_message_mapper.cpp
    persistence/couchbase/couchbase_outbox_repository.cpp
    persistence/couchbase/couchbase_idempotency_repository.cpp
    persistence/couchbase/couchbase_resource_repository.cpp
    persistence/couchbase/couchbase_reservation_repository.cpp
    persistence/couchbase/couchbase_reservation_creation_store.cpp
    persistence/couchbase/couchbase_reservation_creation_event_store.cpp
    persistence/couchbase/metrics/couchbase_persistence_metrics.cpp
    persistence/couchbase/resource_document.cpp
    persistence/couchbase/resource_document_mapper.cpp
    persistence/couchbase/resource_document_validator.cpp
    persistence/couchbase/reservation_document.cpp
    persistence/couchbase/reservation_document_mapper.cpp
    persistence/couchbase/reservation_document_validator.cpp
)

add_library(
    haven::infrastructure::couchbase
    ALIAS
    haven_infrastructure_couchbase
)

target_include_directories(
    haven_infrastructure_couchbase

    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    haven_infrastructure_couchbase

    PUBLIC
        cxx_std_20
)

target_link_libraries(
    haven_infrastructure_couchbase

    PUBLIC
        haven_application
        haven_domain
        couchbase_cxx_client::couchbase_cxx_client

    PRIVATE
        haven_logging
        OpenSSL::Crypto
)

haven_apply_project_options(haven_infrastructure_couchbase)
