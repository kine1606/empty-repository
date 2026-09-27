# Build test executables
# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_sharedfunction

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_sharedfunction

# Test for sharedfunction
test_sharedfunction_SOURCES = SharedFunctionTest.cc

test_sharedfunction_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    $(GTEST_CPPFLAGS) \
    -I$(srcdir)/../inc

test_sharedfunction_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_sharedfunction_LDADD = \
    ../libsharedfunction.la \
    $(GTEST_LIBS) \
    -lgtest_main
