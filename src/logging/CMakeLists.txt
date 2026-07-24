# Haven logging module build configuration.
#
# This file defines the logging library target, its public headers, language
# requirements, and platform threading dependency.

# Build the logging implementation as a reusable static library.
add_library(haven_logging STATIC
    logger.cpp
    scope_trace.cpp
)

# Expose Haven's public headers to targets that link against this module.
target_include_directories(haven_logging
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# Logging uses C++20 language features and exposes C++20 headers publicly.
target_compile_features(haven_logging
    PUBLIC
        cxx_std_20
)

# Thread support is required because log writes are protected by a mutex.
target_link_libraries(haven_logging
    PUBLIC
        Threads::Threads
)