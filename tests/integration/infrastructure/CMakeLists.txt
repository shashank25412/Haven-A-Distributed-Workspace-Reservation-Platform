# Infrastructure integration tests use live external services.

add_executable(
    haven_resource_repository_integration_test

    persistence/couchbase/couchbase_resource_repository_test.cpp
)

target_link_libraries(
    haven_resource_repository_integration_test

    PRIVATE
        haven::infrastructure::couchbase
        GTest::gtest_main
)

haven_apply_project_options(
    haven_resource_repository_integration_test
)

gtest_discover_tests(
    haven_resource_repository_integration_test
    PROPERTIES LABELS "integration;couchbase"
)

add_executable(haven_redis_cache_integration_test
    cache/redis/redis_resource_detail_cache_test.cpp
)
target_link_libraries(haven_redis_cache_integration_test
    PRIVATE haven::infrastructure::redis GTest::gtest_main
)
haven_apply_project_options(haven_redis_cache_integration_test)
gtest_discover_tests(haven_redis_cache_integration_test PROPERTIES LABELS "integration;redis")

add_executable(
    haven_reservation_repository_integration_test

    persistence/couchbase/couchbase_reservation_repository_test.cpp
)

target_link_libraries(
    haven_reservation_repository_integration_test

    PRIVATE
        haven::infrastructure::couchbase
        GTest::gtest_main
)

haven_apply_project_options(haven_reservation_repository_integration_test)

gtest_discover_tests(
    haven_reservation_repository_integration_test
    PROPERTIES LABELS "integration;couchbase"
)

add_executable(haven_reservation_creation_store_integration_test
    persistence/couchbase/couchbase_reservation_creation_store_test.cpp
)
target_link_libraries(haven_reservation_creation_store_integration_test
    PRIVATE haven::infrastructure::couchbase GTest::gtest_main
)
haven_apply_project_options(haven_reservation_creation_store_integration_test)
gtest_discover_tests(haven_reservation_creation_store_integration_test
    PROPERTIES LABELS "integration;couchbase;outbox"
)

add_executable(haven_reservation_creation_event_store_integration_test
    persistence/couchbase/couchbase_reservation_creation_event_store_test.cpp
)
target_link_libraries(haven_reservation_creation_event_store_integration_test
    PRIVATE haven::infrastructure::couchbase GTest::gtest_main
)
haven_apply_project_options(haven_reservation_creation_event_store_integration_test)
gtest_discover_tests(haven_reservation_creation_event_store_integration_test
    PROPERTIES LABELS "integration;couchbase;outbox"
)

add_executable(haven_outbox_repository_integration_test
    persistence/couchbase/couchbase_outbox_repository_test.cpp)
target_link_libraries(haven_outbox_repository_integration_test
    PRIVATE haven::infrastructure::couchbase GTest::gtest_main)
haven_apply_project_options(haven_outbox_repository_integration_test)
gtest_discover_tests(haven_outbox_repository_integration_test
    PROPERTIES LABELS "integration;couchbase;outbox")

add_executable(haven_kafka_outbox_message_producer_integration_test
    messaging/kafka/kafka_outbox_message_producer_test.cpp)
target_link_libraries(haven_kafka_outbox_message_producer_integration_test
    PRIVATE haven::infrastructure::kafka GTest::gtest_main)
haven_apply_project_options(haven_kafka_outbox_message_producer_integration_test)
gtest_discover_tests(haven_kafka_outbox_message_producer_integration_test
    PROPERTIES LABELS "integration;kafka")

add_executable(haven_outbox_pipeline_integration_test
    messaging/outbox/outbox_pipeline_test.cpp)
target_link_libraries(haven_outbox_pipeline_integration_test
    PRIVATE haven::infrastructure::couchbase haven::infrastructure::kafka GTest::gtest_main)
haven_apply_project_options(haven_outbox_pipeline_integration_test)
gtest_discover_tests(haven_outbox_pipeline_integration_test
    PROPERTIES LABELS "integration;couchbase;kafka;outbox-pipeline")

add_executable(haven_idempotency_repository_integration_test
    persistence/couchbase/couchbase_idempotency_repository_test.cpp
)
target_link_libraries(haven_idempotency_repository_integration_test
    PRIVATE haven::infrastructure::couchbase GTest::gtest_main
)
haven_apply_project_options(haven_idempotency_repository_integration_test)
gtest_discover_tests(haven_idempotency_repository_integration_test
    PROPERTIES LABELS "integration;couchbase;idempotency"
)
