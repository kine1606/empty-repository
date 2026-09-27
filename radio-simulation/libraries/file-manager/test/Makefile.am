# Build test executables

# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_filemanager

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_filemanager

# Test for database
test_filemanager_SOURCES = \
    FileManagerTest.cc \
    FileHandlerTest.cc
    
test_filemanager_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    $(GTEST_CPPFLAGS)

test_filemanager_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_filemanager_LDADD = \
    ../libfilemanager.la \
    $(GTEST_LIBS) \
    -lgtest_main
