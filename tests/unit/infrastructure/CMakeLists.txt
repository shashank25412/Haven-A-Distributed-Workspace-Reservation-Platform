# Haven infrastructure unit-test build configuration.
#
# This file defines unit-test executables for infrastructure adapters and
# supporting infrastructure types. Tests requiring a live external service
# belong in the integration-test suite rather than this unit-test directory.

add_executable(
    haven_couchbase_configuration_test

    persistence/couchbase/couchbase_configuration_test.cpp
    persistence/couchbase/couchbase_connection_test.cpp
    persistence/couchbase/couchbase_document_key_test.cpp
    persistence/couchbase/idempotency_persistence_test.cpp
    persistence/couchbase/outbox_document_test.cpp
    persistence/couchbase/outbox_repository_test.cpp
    persistence/couchbase/couchbase_resource_repository_test.cpp
    persistence/couchbase/couchbase_reservation_repository_test.cpp
    persistence/couchbase/couchbase_persistence_metrics_test.cpp
    persistence/couchbase/resource_document_test.cpp
    persistence/couchbase/resource_document_mapper_test.cpp
    persistence/couchbase/resource_document_validator_test.cpp
    persistence/couchbase/reservation_document_test.cpp
    persistence/couchbase/reservation_document_mapper_test.cpp
    persistence/couchbase/reservation_document_validator_test.cpp
)

target_link_libraries(
    haven_couchbase_configuration_test

    PRIVATE
        haven::infrastructure::couchbase
        haven::infrastructure::metrics
        GTest::gtest_main
)
target_include_directories(haven_couchbase_configuration_test PRIVATE ${PROJECT_SOURCE_DIR}/tests/unit)

haven_apply_project_options(haven_couchbase_configuration_test)

gtest_discover_tests(haven_couchbase_configuration_test)

add_executable(haven_redis_cache_tests
    cache/redis/redis_resource_detail_cache_test.cpp
)
target_link_libraries(haven_redis_cache_tests PRIVATE haven::infrastructure::redis GTest::gtest_main)
haven_apply_project_options(haven_redis_cache_tests)
gtest_discover_tests(haven_redis_cache_tests)

add_executable(haven_kafka_tests
    messaging/kafka/kafka_outbox_record_test.cpp
    messaging/kafka/kafka_error_mapper_test.cpp
    messaging/kafka/kafka_outbox_message_producer_metrics_test.cpp
)
target_include_directories(haven_kafka_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests/unit)
target_link_libraries(haven_kafka_tests PRIVATE haven::infrastructure::kafka GTest::gtest_main)
haven_apply_project_options(haven_kafka_tests)
gtest_discover_tests(haven_kafka_tests PROPERTIES LABELS "kafka")
