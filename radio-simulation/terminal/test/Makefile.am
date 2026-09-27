TESTS = test_terminal

check_PROGRAMS = test_terminal

test_terminal_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/../inc \
    -I$(top_srcdir)/libraries/mailbox/inc \
    -I$(top_srcdir)/libraries/mailbox/inc/manager \
    -I$(top_srcdir)/libraries/mailbox/inc/support \
    -I$(top_srcdir)/libraries/mailbox/inc/validation \
    -I$(top_srcdir)/libraries/mailbox/inc/error \
    -I$(top_srcdir)/libraries/grpc/inc \
    -I$(top_srcdir)/libraries/proto \
    -I$(top_srcdir)/libraries/logger/inc \
    -I$(top_srcdir)/libraries/file-manager/inc \
    $(PROTOBUF_CFLAGS) \
    $(GRPC_CFLAGS) \
    $(READLINE_CFLAGS) \
    $(GTEST_CPPFLAGS)

test_terminal_SOURCES = TerminalTest.cc

test_terminal_CXXFLAGS = -std=c++20

test_terminal_LDADD = \
    ../libterminal.la \
    $(top_builddir)/libraries/grpc/libnode_grpc.la \
    $(top_builddir)/libraries/mailbox/libmailbox.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la \
    $(top_builddir)/libraries/logger/liblogger.la \
    $(top_builddir)/libraries/file-manager/libfilemanager.la \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS) \
    $(READLINE_LIBS) \
    $(GTEST_LIBS) \
    -lgtest_main
