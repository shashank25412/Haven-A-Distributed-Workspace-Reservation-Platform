# Haven application unit test configuration.
#
# Application tests use in-memory fakes and do not require external
# infrastructure or presentation frameworks.

add_executable(haven_application_tests
    health/readiness_test.cpp
    outbox_message_producer_test.cpp
    outbox/outbox_publisher_test.cpp
    idempotency/idempotency_contract_test.cpp
    idempotency/test_idempotency_repository_test.cpp

    resources/get_resource_handler_test.cpp
    resources/cached_resource_query_repository_test.cpp
    resources/search_resources_handler_test.cpp
    resources/search_available_resources_handler_test.cpp
    resources/get_resource_calendar_handler_test.cpp
    resources/test_resource_repository_test.cpp

    reservations/create_reservation_handler_test.cpp
    reservations/reservation_creation_store_test.cpp
    reservations/get_reservation_handler_test.cpp
    reservations/list_caller_reservations_handler_test.cpp
    reservations/cancel_reservation_handler_test.cpp
    reservations/extend_reservation_handler_test.cpp
    reservations/list_pending_approvals_handler_test.cpp
    reservations/approve_reservation_handler_test.cpp
    reservations/reject_reservation_handler_test.cpp
    reservations/test_reservation_repository_test.cpp
)

target_include_directories(haven_application_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/unit
)

target_link_libraries(haven_application_tests
    PRIVATE
        haven::application
        GTest::gtest_main
)

haven_apply_project_options(haven_application_tests)

include(GoogleTest)

gtest_discover_tests(haven_application_tests)

add_executable(haven_metrics_foundation_tests
    observability/metrics/metrics_foundation_test.cpp
    observability/metrics/prometheus_metrics_recorder_test.cpp
)

target_include_directories(haven_metrics_foundation_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/unit
)

target_link_libraries(haven_metrics_foundation_tests
    PRIVATE
        haven::infrastructure::metrics
        haven::infrastructure::kafka
        haven::infrastructure::couchbase
        haven::infrastructure::redis
        GTest::gtest_main
)

haven_apply_project_options(haven_metrics_foundation_tests)

gtest_discover_tests(haven_metrics_foundation_tests)
