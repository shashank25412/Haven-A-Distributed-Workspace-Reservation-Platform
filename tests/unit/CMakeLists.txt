#------------------------------------------------------------------------------
# Haven Unit Tests
#
# Defines fast, isolated tests that do not require external infrastructure.
#------------------------------------------------------------------------------

add_executable(
    haven_unit_tests

    presentation/health/live_response_test.cpp
)

target_link_libraries(
    haven_unit_tests

    PRIVATE
        haven::presentation
        GTest::gtest_main
)

haven_apply_project_options(haven_unit_tests)

include(GoogleTest)

gtest_discover_tests(haven_unit_tests)