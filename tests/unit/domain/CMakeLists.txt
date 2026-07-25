# Haven domain unit tests.
#
# Domain tests verify framework-independent value objects, aggregates,
# policies, and business rules.

# These tests verify identifier invariants, version semantics, and half-open
# reservation interval behaviour.

add_executable(
    haven_domain_value_objects_tests

    events/reservation_created_event_test.cpp
    events/reservation_approval_requested_event_test.cpp
    events/reservation_confirmed_event_test.cpp
    events/reservation_rejected_event_test.cpp
    events/reservation_cancelled_event_test.cpp
    events/reservation_extended_event_test.cpp
    events/reservation_expired_event_test.cpp
    events/reservation_completed_event_test.cpp

    policies/reservation_policy_test.cpp
    policies/reservation_conflict_policy_test.cpp
    policies/reservation_creation_policy_test.cpp

    value_objects/event_id_test.cpp
    value_objects/idempotency_key_test.cpp
    value_objects/organization_id_test.cpp
    value_objects/reservation_id_test.cpp
    value_objects/resource_id_test.cpp
    value_objects/time_interval_test.cpp
    value_objects/user_id_test.cpp
    value_objects/version_test.cpp
    value_objects/purpose_test.cpp
    value_objects/resource_type_test.cpp
    value_objects/resource_status_test.cpp
    value_objects/reservation_status_test.cpp
    value_objects/reservation_kind_test.cpp
    value_objects/approval_info_test.cpp

    resource_test.cpp
    reservation_test.cpp
)

target_link_libraries(
    haven_domain_value_objects_tests

    PRIVATE
        haven::domain
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(haven_domain_value_objects_tests)
