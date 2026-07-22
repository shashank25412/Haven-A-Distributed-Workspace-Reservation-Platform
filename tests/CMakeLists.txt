#------------------------------------------------------------------------------
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
#------------------------------------------------------------------------------

add_subdirectory(unit)