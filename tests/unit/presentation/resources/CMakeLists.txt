# Haven Resource presentation tests.

add_executable(
    haven_resource_response_test

    resource_response_test.cpp
)

target_link_libraries(
    haven_resource_response_test

    PRIVATE
        haven::presentation
        GTest::gtest_main
)

haven_apply_project_options(haven_resource_response_test)
gtest_discover_tests(haven_resource_response_test)

# Exercise the manually registered route through a real local HTTP boundary.
add_executable(
    haven_resource_controller_test

    main.cpp
    get_resource_controller_test.cpp
)

target_include_directories(
    haven_resource_controller_test

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(
    haven_resource_controller_test

    PRIVATE
        haven::presentation
        Drogon::Drogon
        Threads::Threads
)

haven_apply_project_options(haven_resource_controller_test)

add_test(
    NAME haven_resource_controller_test
    COMMAND haven_resource_controller_test
)
