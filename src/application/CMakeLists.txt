# Haven application layer build configuration.
#
# This target contains application-owned ports and use-case handlers.
# Application code depends on the domain and logging modules without
# introducing presentation or infrastructure dependencies.

add_library(haven_application
    message_publish_error.cpp
    repository_error.cpp

    outbox/outbox_publisher.cpp
    outbox/system_outbox_publisher_clock.cpp

    idempotency/create_reservation_fingerprint.cpp
    idempotency/create_reservation_result_snapshot.cpp
    idempotency/idempotency_fingerprint.cpp
    idempotency/idempotency_record.cpp

    resources/get_resource_handler.cpp
    resources/authoritative_resource_query_repository.cpp
    resources/cached_resource_query_repository.cpp
    resources/search_resources_handler.cpp
    resources/search_available_resources_handler.cpp
    resources/get_resource_calendar_handler.cpp

    reservations/create_reservation_handler.cpp
    reservations/get_reservation_handler.cpp
    reservations/list_caller_reservations_handler.cpp
    reservations/cancel_reservation_handler.cpp
    reservations/extend_reservation_handler.cpp
    reservations/list_pending_approvals_handler.cpp
    reservations/approve_reservation_handler.cpp
    reservations/reject_reservation_handler.cpp
)

find_package(OpenSSL REQUIRED)

add_library(haven::application ALIAS haven_application)

target_include_directories(
    haven_application

    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    haven_application

    PUBLIC
        haven::domain
    PRIVATE
        haven_logging
        OpenSSL::Crypto
)

target_compile_features(haven_application
    PUBLIC
        cxx_std_20
)
