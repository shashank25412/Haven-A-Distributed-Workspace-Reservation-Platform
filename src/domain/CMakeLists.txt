# Haven domain model.
#
# This target contains framework-independent business types and rules.
# Infrastructure, transport, and external SDK dependencies must not be added
# to this target.

add_library(
    haven_domain STATIC

    events/reservation_created_event.cpp
    events/reservation_approval_requested_event.cpp
    events/reservation_confirmed_event.cpp
    events/reservation_rejected_event.cpp
    events/reservation_cancelled_event.cpp
    events/reservation_extended_event.cpp
    events/reservation_expired_event.cpp
    events/reservation_completed_event.cpp

    policies/reservation_policy.cpp
    policies/reservation_conflict_policy.cpp
    policies/reservation_creation_policy.cpp

    value_objects/event_id.cpp
    value_objects/idempotency_key.cpp
    value_objects/organization_id.cpp
    value_objects/reservation_id.cpp
    value_objects/resource_id.cpp
    value_objects/time_interval.cpp
    value_objects/user_id.cpp
    value_objects/purpose.cpp
    value_objects/resource_type.cpp
    value_objects/resource_status.cpp
    value_objects/reservation_status.cpp
    value_objects/reservation_kind.cpp
    value_objects/approval_info.cpp

    resource.cpp
    reservation.cpp
)

add_library(
    haven::domain ALIAS haven_domain)

target_include_directories(haven_domain
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(haven_domain
    PUBLIC
        cxx_std_20
)

target_link_libraries(haven_domain
    PRIVATE
        haven_logging
)
