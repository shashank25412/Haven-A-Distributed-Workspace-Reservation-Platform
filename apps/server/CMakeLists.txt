#------------------------------------------------------------------------------
# Haven Server Executable
#
# Defines the Haven API process entry point.
#
# The executable acts as the composition root. It loads validated process
# configuration, registers presentation routes, and starts the HTTP framework.
#
# Domain and application business logic must not be implemented here.
#------------------------------------------------------------------------------

add_executable(
    haven_server
    main.cpp
)

target_link_libraries(
    haven_server

    PRIVATE
        haven::bootstrap
        haven::presentation
        Drogon::Drogon
)

# Keep the CMake target identifier underscore-based while producing a
# conventional command name for local execution and deployment.
set_target_properties(
    haven_server

    PROPERTIES
        OUTPUT_NAME "haven-server"
)

haven_apply_project_options(haven_server)

# Install the executable into a predictable deployment directory.
#
# The Docker build later installs Haven with:
#
#     cmake --install build/release --prefix /opt/haven
#
# producing:
#
#     /opt/haven/bin/haven-server
install(
    TARGETS haven_server
    RUNTIME DESTINATION bin
)