#------------------------------------------------------------------------------
# Haven Server Executable
#
# Defines the Haven API process entry point.
#
# The executable acts as the composition root. It starts the HTTP framework and
# connects project modules, but it must not contain domain or application logic.
#------------------------------------------------------------------------------

add_executable(
    haven_server
    main.cpp
)

target_link_libraries(
    haven_server
    PRIVATE
        haven::presentation
        Drogon::Drogon
)

# Produce a conventional executable name while retaining a CMake-friendly
# target identifier.
set_target_properties(
    haven_server
    PROPERTIES
        OUTPUT_NAME "haven-server"
)

haven_apply_project_options(haven_server)

# Prepare the executable for installation into a container or deployment
# package without encoding a platform-specific destination.
install(
    TARGETS haven_server
    RUNTIME DESTINATION bin
)