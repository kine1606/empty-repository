TESTS = test_ru

check_PROGRAMS = test_ru

test_ru_CPPFLAGS = \
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
    $(GTEST_CPPFLAGS)

test_ru_SOURCES = RadioUnitTest.cc

test_ru_CXXFLAGS = -std=c++20

test_ru_LDADD = \
    ../libru.la \
    $(top_builddir)/libraries/grpc/libnode_grpc.la \
    $(top_builddir)/libraries/mailbox/libmailbox.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la \
    $(top_builddir)/libraries/logger/liblogger.la \
    $(top_builddir)/libraries/file-manager/libfilemanager.la \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS) \
    $(GTEST_LIBS) \
    -lgtest_main
