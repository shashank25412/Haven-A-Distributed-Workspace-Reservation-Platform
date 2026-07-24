# Haven health presentation tests.

add_executable(
    haven_live_response_test

    live_response_test.cpp
)

target_link_libraries(
    haven_live_response_test

    PRIVATE
        haven::presentation
        GTest::gtest_main
)

haven_apply_project_options(haven_live_response_test)

gtest_discover_tests(haven_live_response_test)

# The controller test uses Drogon's asynchronous test framework and therefore
# supplies its own runner rather than linking GTest::gtest_main.
add_executable(
    haven_live_controller_test

    main.cpp
    live_controller_test.cpp
)

target_link_libraries(
    haven_live_controller_test

    PRIVATE
        haven::presentation
        Drogon::Drogon
        Threads::Threads
)

haven_apply_project_options(haven_live_controller_test)

add_test(
    NAME haven_live_controller_test
    COMMAND haven_live_controller_test
)
