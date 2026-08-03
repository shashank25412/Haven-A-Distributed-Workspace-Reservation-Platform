# Haven runtime unit tests.
add_executable(haven_outbox_runtime_tests outbox/outbox_publisher_worker_test.cpp)
target_link_libraries(haven_outbox_runtime_tests
    PRIVATE haven::runtime::outbox GTest::gtest_main)
haven_apply_project_options(haven_outbox_runtime_tests)
gtest_discover_tests(haven_outbox_runtime_tests)
