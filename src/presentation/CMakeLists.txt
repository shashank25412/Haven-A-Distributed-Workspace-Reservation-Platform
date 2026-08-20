#------------------------------------------------------------------------------
# Haven Presentation Module
#
# Owns the HTTP presentation layer:
#
#   • route registration
#   • controllers
#   • HTTP DTOs
#   • request/response translation
#
# The presentation layer depends on Drogon and Haven's logging module today.
# Business workflows will be delegated to the application layer as the project
# evolves.
#------------------------------------------------------------------------------

add_library(
    haven_presentation
    STATIC

    health/live_controller.cpp
    health/live_response.cpp
    health/readiness_controller.cpp
    health/readiness_response.cpp

    observability/metrics/metrics_controller.cpp

    api_error_response.cpp
    http_timestamp.cpp

    auth/authentication_controller.cpp

    approvals/approve_reservation_controller.cpp
    approvals/list_pending_approvals_controller.cpp
    approvals/list_decided_approvals_controller.cpp
    approvals/reject_reservation_controller.cpp

    reservations/create_reservation_controller.cpp
    reservations/create_reservation_request.cpp
    reservations/create_reservation_response.cpp
    reservations/list_my_reservations_controller.cpp

    admin/list_all_reservations_controller.cpp
    admin/admin_cancel_reservation_controller.cpp

    resources/get_resource_controller.cpp
    resources/search_resources_controller.cpp
    resources/resource_error_response.cpp
    resources/resource_response.cpp

    organizations/list_organizations_controller.cpp
    organizations/organization_response.cpp
)

# Preferred namespace-qualified target used throughout the project.
add_library(
    haven::presentation
    ALIAS
    haven_presentation
)

target_include_directories(
    haven_presentation

    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    haven_presentation

    PRIVATE
        haven_logging
        Drogon::Drogon

    PUBLIC
        haven::application
)

# Apply Haven compiler policies.
haven_apply_project_options(haven_presentation)
