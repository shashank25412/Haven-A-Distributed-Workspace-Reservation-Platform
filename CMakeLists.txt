# Haven root build configuration.
#
# This file defines project-wide build requirements and connects the
# independently owned application, presentation, and test targets.
# Target-specific source files and dependencies belong in their respective subdirectory CMakeLists.txt files.

cmake_minimum_required(VERSION 3.28)

project(
    Haven
    VERSION 0.1.0
    DESCRIPTION "Multi-tenant shared-resource reservation backend"
    LANGUAGES CXX
)

# Haven requires standard C++20 behavior.
# Compiler-specific language extensions are disabled to improve portability.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clang-tidy, IDE integration,
# and other static-analysis tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(
    HVN_BUILD_TESTS
    "Build Haven unit and integration test targets"
    ON
)

option(
    HVN_ENABLE_WARNINGS_AS_ERRORS
    "Treat compiler warnings in Haven-owned targets as errors"
    OFF
)

option(
    HVN_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

# Reusable project-owned CMake helpers live here.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(HavenCompilerOptions)

# Drogon is required by the presentation layer and server executable.
# It is restored through the project's vcpkg manifest.
find_package(Drogon CONFIG REQUIRED)

# Discover the platform threading library required by the logging module.
find_package(Threads REQUIRED)

# Build Haven-owned libraries before the server executable.
# The server acts as the composition root and links these module targets.
add_subdirectory(src/logging)
add_subdirectory(src/bootstrap)
add_subdirectory(src/presentation)
add_subdirectory(apps/server)

if(HVN_BUILD_TESTS)
    include(CTest)
    enable_testing()

    # GoogleTest is required only when test targets are enabled.
    find_package(GTest CONFIG REQUIRED)

    add_subdirectory(tests)
endif()