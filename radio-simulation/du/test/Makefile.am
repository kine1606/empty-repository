# Build test executables
# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_du

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_du

# Test for du
# test_du_SOURCES = TestDU.cc
test_du_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    $(GTEST_CPPFLAGS)

test_du_SOURCES = DistributedUnitTest.cc

test_du_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_du_LDADD = \
    ../libdu.la \
    $(GTEST_LIBS) \
    -lgtest_main
