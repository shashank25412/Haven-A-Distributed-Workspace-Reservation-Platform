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

add_library(haven_infrastructure_redis
    cache/redis/redis_connection.cpp
    cache/redis/resource_detail_cache_key.cpp
    cache/redis/cached_resource_record.cpp
    cache/redis/redis_resource_detail_cache.cpp
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
    persistence/couchbase/couchbase_idempotency_repository.cpp
    persistence/couchbase/couchbase_resource_repository.cpp
    persistence/couchbase/couchbase_reservation_repository.cpp
    persistence/couchbase/couchbase_reservation_creation_store.cpp
    persistence/couchbase/couchbase_reservation_creation_event_store.cpp
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
