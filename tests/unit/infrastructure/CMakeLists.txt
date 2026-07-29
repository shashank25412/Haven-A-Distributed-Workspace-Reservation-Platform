# Haven infrastructure unit-test build configuration.
#
# This file defines unit-test executables for infrastructure adapters and
# supporting infrastructure types. Tests requiring a live external service
# belong in the integration-test suite rather than this unit-test directory.

add_executable(
    haven_couchbase_configuration_test

    persistence/couchbase/couchbase_configuration_test.cpp
    persistence/couchbase/couchbase_connection_test.cpp
    persistence/couchbase/couchbase_document_key_test.cpp
    persistence/couchbase/resource_document_test.cpp
    persistence/couchbase/resource_document_mapper_test.cpp
    persistence/couchbase/resource_document_validator_test.cpp
)

target_link_libraries(
    haven_couchbase_configuration_test

    PRIVATE
        haven::infrastructure::couchbase
        GTest::gtest_main
)

haven_apply_project_options(haven_couchbase_configuration_test)

gtest_discover_tests(haven_couchbase_configuration_test)
