lib_LTLIBRARIES = liblogger.la

#Where source files are
liblogger_la_SOURCES = \
    src/Logger.cc

#Where compiler can find the header
liblogger_la_CPPFLAGS = \
    -I$(srcdir)/inc \
    -I$(srcdir)/../file-manager/inc

liblogger_la_LIBADD = \
    $(top_builddir)/libraries/file-manager/libfilemanager.la
    
SUBDIRS = \
    . \
    test
