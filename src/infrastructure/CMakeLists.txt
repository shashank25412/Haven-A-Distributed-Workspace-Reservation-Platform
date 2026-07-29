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

add_library(
    haven_infrastructure_couchbase

    persistence/couchbase/couchbase_connection.cpp
    persistence/couchbase/couchbase_document_key.cpp
    persistence/couchbase/couchbase_resource_repository.cpp
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
)
