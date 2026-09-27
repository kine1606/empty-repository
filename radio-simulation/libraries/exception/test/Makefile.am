# Build test executables

# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_exception

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_exception

test_exception_SOURCES = \
    ExceptionTest.cc \
    FileManagerExceptionTest.cc \
    DatabaseExceptionTest.cc
# Test for exception
test_exception_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    -I$(srcdir)/../../enums/ \
    $(GTEST_CPPFLAGS)

test_exception_CXXFLAGS = -std=c++20 

# Link against:
# - Our library
# - Google Test libraries
test_exception_LDADD = \
    ../libexception.la \
    $(GTEST_LIBS) \
    -lgtest_main
