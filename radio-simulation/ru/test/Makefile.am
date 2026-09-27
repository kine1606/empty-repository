# Build test executables

# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_ru

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_ru

# Test for ru
test_ru_SOURCES = RadioUnitTest.cc

test_ru_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    $(GTEST_CPPFLAGS)

test_ru_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_ru_LDADD = \
    ../libru.la \
    $(GTEST_LIBS) \
    -lgtest_main
