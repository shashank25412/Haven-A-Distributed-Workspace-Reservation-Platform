# Server-level Reservation Transactional Outbox integration tests.

add_executable(haven_reservation_outbox_server_test
    reservation_outbox_server_test.cpp)

target_link_libraries(haven_reservation_outbox_server_test
    PRIVATE
        haven::infrastructure::couchbase
        haven::infrastructure::kafka
        Drogon::Drogon
        GTest::gtest_main)

target_compile_definitions(haven_reservation_outbox_server_test
    PRIVATE HAVEN_SERVER_PATH="$<TARGET_FILE:haven_server>")

add_dependencies(haven_reservation_outbox_server_test haven_server)

haven_apply_project_options(haven_reservation_outbox_server_test)

gtest_discover_tests(haven_reservation_outbox_server_test
    PROPERTIES LABELS "acceptance;integration;couchbase;kafka;http;outbox")
