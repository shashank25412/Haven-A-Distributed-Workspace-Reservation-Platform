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
