# Build test executables

# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_database

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_database

# Test for database
test_database_SOURCES = DatabaseTest.cc
test_database_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    -I$(srcdir)/../../exception/inc \
    $(GTEST_CPPFLAGS)

test_database_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_database_LDADD = \
    ../libdatabase.la \
    ../../exception/libexception.la \
    $(GTEST_LIBS) \
    -lgtest_main
