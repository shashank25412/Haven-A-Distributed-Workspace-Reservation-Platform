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
# The presentation layer depends only on Drogon today.
# Business workflows will be delegated to the application layer as the project
# evolves.
#------------------------------------------------------------------------------

add_library(
    haven_presentation
    STATIC

    health/live_controller.cpp
    health/live_response.cpp
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
        Drogon::Drogon
)

# Apply Haven compiler policies.
haven_apply_project_options(haven_presentation)