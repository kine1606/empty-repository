AM_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(srcdir)/inc \
    -I$(top_srcdir)/libraries/proto \
    -I$(top_srcdir)/libraries/mailbox/inc \
    -I$(top_srcdir)/libraries/mailbox/inc/manager \
    -I$(top_srcdir)/libraries/mailbox/inc/support \
    -I$(top_srcdir)/libraries/mailbox/inc/validation \
    -I$(top_srcdir)/libraries/node/inc \
    -I$(top_srcdir)/libraries/database/inc \
    -I$(top_srcdir)/libraries/shared-function/inc \
    $(PROTOBUF_CFLAGS) \
    $(GRPC_CFLAGS)
AM_CXXFLAGS = \
    -std=c++17 \
    -Wall \
    -Wextra
noinst_LTLIBRARIES = libnode_grpc.la
libnode_grpc_la_SOURCES = \
    inc/GrpcServer.h \
    inc/NodeClient.h \
    inc/NodeServiceImpl.h \
    src/GrpcServer.cc \
    src/NodeClient.cc \
    src/NodeServiceImpl.cc
libnode_grpc_la_LIBADD = \
    $(top_builddir)/libraries/mailbox/libmailbox.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la \
    $(top_builddir)/libraries/database/libdatabase.la \
    $(top_builddir)/libraries/shared-function/libsharedfunction.la \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS)
