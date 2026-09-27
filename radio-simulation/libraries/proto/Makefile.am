AM_CPPFLAGS = \
    -I$(srcdir) \
    $(PROTOBUF_CFLAGS) \
    $(GRPC_CFLAGS)
AM_CXXFLAGS = -std=c++17
noinst_LTLIBRARIES = libmailbox_proto.la
nodist_libmailbox_proto_la_SOURCES = \
    du.pb.cc \
    mailbox.grpc.pb.cc \
    mailbox.pb.cc \
    ru.pb.cc \
    terminal.pb.cc
libmailbox_proto_la_LIBADD = \
    $(PROTOBUF_LIBS) \
    $(GRPC_LIBS)
BUILT_SOURCES = \
    $(srcdir)/du.pb.cc \
    $(srcdir)/du.pb.h \
    $(srcdir)/mailbox.grpc.pb.cc \
    $(srcdir)/mailbox.grpc.pb.h \
    $(srcdir)/mailbox.pb.cc \
    $(srcdir)/mailbox.pb.h \
    $(srcdir)/ru.pb.cc \
    $(srcdir)/ru.pb.h \
    $(srcdir)/terminal.pb.cc \
    $(srcdir)/terminal.pb.h
EXTRA_DIST = \
    du.proto \
    mailbox.proto \
    ru.proto \
    terminal.proto
CLEANFILES = protobuf.stamp

# Keep generated C++ beside the contract; compile all objects in the build tree.
protobuf.stamp: $(srcdir)/du.proto $(srcdir)/mailbox.proto $(srcdir)/ru.proto $(srcdir)/terminal.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) --grpc_out=$(srcdir) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(srcdir)/mailbox.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) $(srcdir)/du.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) $(srcdir)/ru.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) $(srcdir)/terminal.proto
	touch $@

$(BUILT_SOURCES): protobuf.stamp
	@test -f $@ || { rm -f protobuf.stamp; $(MAKE) protobuf.stamp; }
