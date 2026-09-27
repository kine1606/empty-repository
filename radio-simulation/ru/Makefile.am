bin_PROGRAMS = ru_simulation

# Build ru as a library
lib_LTLIBRARIES = libru.la
libru_la_SOURCES = src/RadioUnit.cc
libru_la_CPPFLAGS = \
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
    $(GRPC_CFLAGS)
libru_la_CXXFLAGS = -std=c++20

ru_simulation_SOURCES = src/main.cc

ru_simulation_CPPFLAGS = \
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
    $(GRPC_CFLAGS)

ru_simulation_CXXFLAGS = -std=c++20

ru_simulation_LDADD = \
    $(builddir)/libru.la \
    $(top_builddir)/libraries/grpc/libnode_grpc.la \
    $(top_builddir)/libraries/mailbox/libmailbox.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la \
    $(top_builddir)/libraries/logger/liblogger.la \
    $(top_builddir)/libraries/file-manager/libfilemanager.la \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS)

SUBDIRS = \
    . \
    test
