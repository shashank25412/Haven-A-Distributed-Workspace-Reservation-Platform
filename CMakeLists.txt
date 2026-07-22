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
    HAVEN_BUILD_TESTS
    "Build Haven unit and integration test targets"
    ON
)

option(
    HAVEN_ENABLE_WARNINGS_AS_ERRORS
    "Treat compiler warnings in Haven-owned targets as errors"
    OFF
)

option(
    HAVEN_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

# Reusable project-owned CMake helpers live here.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(HavenCompilerOptions)

# Drogon is required by the presentation layer and server executable.
# It is restored through the project's vcpkg manifest.
find_package(Drogon CONFIG REQUIRED)

# Build the presentation library before the server, because the server
# depends on presentation-layer route registration.
add_subdirectory(src/presentation)
add_subdirectory(apps/server)

if(HAVEN_BUILD_TESTS)
    include(CTest)
    enable_testing()

    # GoogleTest is required only when test targets are enabled.
    find_package(GTest CONFIG REQUIRED)

    add_subdirectory(tests)
endif()