# Haven bootstrap unit tests.

add_executable(
    haven_configuration_test

    configuration_test.cpp
)

target_link_libraries(
    haven_configuration_test

    PRIVATE
        haven::bootstrap
        GTest::gtest_main
)

haven_apply_project_options(haven_configuration_test)

gtest_discover_tests(haven_configuration_test)
