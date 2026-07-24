# Haven logging unit tests.

add_executable(
    haven_logging_tests

    logging_test.cpp
)

target_link_libraries(
    haven_logging_tests

    PRIVATE
        haven_logging
        GTest::gtest_main
)

haven_apply_project_options(haven_logging_tests)

gtest_discover_tests(haven_logging_tests)
