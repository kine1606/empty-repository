# Build test executables

# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = \
    test_logger_sync \
    test_logger_async

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = \
    test_logger_sync \
    test_logger_async

test_logger_sync_SOURCES = LoggerTestSync.cc

# Test for logger
test_logger_sync_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    -I$(srcdir)/../../file-manager/inc \
    $(GTEST_CPPFLAGS)

test_logger_sync_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries

test_logger_sync_LDADD = \
    ../liblogger.la \
    $(GTEST_LIBS) \
    -lgtest_main

test_logger_async_SOURCES = LoggerTestAsync.cc

# Test for logger
test_logger_async_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    -I$(srcdir)/../../file-manager/inc \
    $(GTEST_CPPFLAGS)

test_logger_async_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries

test_logger_async_LDADD = \
    ../liblogger.la \
    $(GTEST_LIBS) \
    -lgtest_main
