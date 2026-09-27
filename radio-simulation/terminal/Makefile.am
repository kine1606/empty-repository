bin_PROGRAMS = terminal_simulation

# Build terminal as a library
lib_LTLIBRARIES = libterminal.la
libterminal_la_SOURCES = \
    src/Terminal.cc

libterminal_la_CPPFLAGS = \
    -I$(srcdir)/inc \
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
    $(READLINE_CFLAGS)

libterminal_la_CXXFLAGS = -std=c++20

libterminal_la_LIBADD = \
    $(READLINE_LIBS)

terminal_simulation_SOURCES = \
    src/main.cc

terminal_simulation_CPPFLAGS = \
    -I$(srcdir)/inc \
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
    $(READLINE_CFLAGS)

terminal_simulation_CXXFLAGS = -std=c++20

terminal_simulation_LDADD = \
    $(builddir)/libterminal.la \
    $(top_builddir)/libraries/grpc/libnode_grpc.la \
    $(top_builddir)/libraries/mailbox/libmailbox.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la \
    $(top_builddir)/libraries/logger/liblogger.la \
    $(top_builddir)/libraries/file-manager/libfilemanager.la \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS) \
    $(READLINE_LIBS)

SUBDIRS = \
    . \
    test
