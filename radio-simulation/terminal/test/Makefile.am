# Build test executables
# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_terminal

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_terminal

test_terminal_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    $(GTEST_CPPFLAGS)

test_terminal_SOURCES = TerminalTest.cc

test_terminal_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_terminal_LDADD = \
    ../libterminal.la \
    $(GTEST_LIBS) \
    -lgtest_main
