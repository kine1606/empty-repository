AM_CPPFLAGS = \
    -I$(srcdir) \
    $(PROTOBUF_CFLAGS) \
    $(GRPC_CFLAGS)
AM_CXXFLAGS = -std=c++17
noinst_LTLIBRARIES = libmailbox_proto.la
nodist_libmailbox_proto_la_SOURCES = \
    mailbox.grpc.pb.cc \
    mailbox.pb.cc
libmailbox_proto_la_LIBADD = \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS)
BUILT_SOURCES = \
    $(srcdir)/mailbox.grpc.pb.cc \
    $(srcdir)/mailbox.grpc.pb.h \
    $(srcdir)/mailbox.pb.cc \
    $(srcdir)/mailbox.pb.h
EXTRA_DIST = mailbox.proto
CLEANFILES = protobuf.stamp

# Keep generated C++ beside the contract; compile all objects in the build tree.
protobuf.stamp: $(srcdir)/mailbox.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) --grpc_out=$(srcdir) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(srcdir)/mailbox.proto
	touch $@

$(BUILT_SOURCES): protobuf.stamp
	@test -f $@ || { rm -f protobuf.stamp; $(MAKE) protobuf.stamp; }
