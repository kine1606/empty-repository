lib_LTLIBRARIES = \
    libmailbox.la

#Where source files are
libmailbox_la_SOURCES = \
    src/Mailbox.cc \
    src/MailboxWorker.cc \
    src/manager/BaseMailboxManager.cc \
    src/manager/NodeAManager.cc \
    src/manager/NodeBManager.cc \
    src/validation/BaseValidator.cc \
    src/validation/ValidationResult.cc

#Where compiler can find the header
libmailbox_la_CPPFLAGS = \
    -I$(srcdir)/inc \
    -I$(srcdir)/inc/manager \
    -I$(srcdir)/inc/support \
    -I$(srcdir)/inc/validation \
    -I$(srcdir)/inc/error \
    -I$(top_srcdir)/libraries/logger/inc \
    -I$(top_srcdir)/libraries/file-manager/inc \
    -I$(top_srcdir)/libraries/proto

libmailbox_la_LIBADD = \
    $(top_builddir)/libraries/logger/liblogger.la \
    $(top_builddir)/libraries/proto/libmailbox_proto.la

SUBDIRS = \
    . \
    test
