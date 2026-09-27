bin_PROGRAMS = du_simulation

# Build du as a library
lib_LTLIBRARIES = libdu.la
libdu_la_SOURCES = src/DistributedUnit.cc
libdu_la_CPPFLAGS = \
    -I$(srcdir)/inc

du_simulation_SOURCES = src/main.cc

du_simulation_CPPFLAGS = \
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

du_simulation_CXXFLAGS = -std=c++20

du_simulation_LDADD = \
    $(builddir)/libdu.la \
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
