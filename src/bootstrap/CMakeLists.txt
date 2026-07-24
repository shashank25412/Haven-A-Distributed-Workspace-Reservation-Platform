#------------------------------------------------------------------------------
# Haven Bootstrap Module
#
# Owns process-level startup concerns such as:
#
#   • loading and validating runtime configuration
#   • constructing application dependencies
#   • coordinating startup and shutdown
#
# The bootstrap layer must not contain domain or application business logic.
#------------------------------------------------------------------------------

add_library(
    haven_bootstrap
    STATIC

    configuration.cpp
)

# Namespace-qualified target used by Haven consumers.
add_library(
    haven::bootstrap
    ALIAS
    haven_bootstrap
)

target_include_directories(
    haven_bootstrap

    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

haven_apply_project_options(haven_bootstrap)