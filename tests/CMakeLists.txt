# Haven Test Suite
#
# This directory owns all automated tests.
#
# Tests are grouped by purpose rather than by production module:
#
#   • unit          Fast isolated tests
#   • integration   Real infrastructure tests
#   • concurrency   Transaction and race-condition validation
#   • e2e           End-to-end API verification
#

# Provides gtest_discover_tests() to all GoogleTest-based test directories.
include(GoogleTest)

# Defer test enumeration until CTest runs. Several integration executables
# load external-service SDKs, so executing them as post-build discovery steps
# can make an otherwise successful build depend on the local runtime setup.
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

# Some system-installed SDK targets expose a broad prefix such as
# /opt/homebrew/include. Keep the vcpkg GoogleTest include directory ahead of
# those prefixes so test sources cannot pick up headers from a different
# GoogleTest installation than the library supplied by GTest::gtest.
get_target_property(
    HVN_GTEST_INCLUDE_DIRECTORIES
    GTest::gtest
    INTERFACE_INCLUDE_DIRECTORIES
)
include_directories(BEFORE "${HVN_GTEST_INCLUDE_DIRECTORIES}")

add_subdirectory(unit)
add_subdirectory(integration)
