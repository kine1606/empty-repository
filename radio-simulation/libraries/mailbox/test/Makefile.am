# Build test executables
# Tell 'make check' how to run the test
# The test executable returns 0 on success, non-zero on failure
TESTS = test_mailbox

# Using check_PROGRAMS instead of bin_PROGRAMS means these are only built with 'make check'
check_PROGRAMS = test_mailbox

test_mailbox_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    -I$(srcdir)/../inc/error \
    -I$(srcdir)/../inc/manager \
    -I$(srcdir)/../inc/support \
    -I$(srcdir)/../inc/validation \
    -I$(top_srcdir)/libraries/logger/inc \
    -I$(top_srcdir)/libraries/file-manager/inc \
    -I$(top_srcdir)/libraries/proto \
    $(GTEST_CPPFLAGS)

test_mailbox_SOURCES = \
    MailboxTest.cc \
    ValidatorTest.cc \
    MailboxDemoTest.cc

test_mailbox_CXXFLAGS = -std=c++20

# Link against:
# - Our library
# - Google Test libraries
test_mailbox_LDADD = \
    ../libmailbox.la \
    $(top_builddir)/libraries/logger/liblogger.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la \
    $(GTEST_LIBS) \
    -lgtest_main