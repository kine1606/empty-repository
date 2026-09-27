lib_LTLIBRARIES = libdatabase.la

#Where source files are
libdatabase_la_SOURCES = src/Database.cc

#Where compiler can find the headers
libdatabase_la_CPPFLAGS = \
    -I$(srcdir)/inc \
    -I$(srcdir)/../file-manager/inc \
    -I$(srcdir)/../exception/inc

#Libraries required by Database
libdatabase_la_LIBADD = \
    ../file-manager/libfilemanager.la \
    ../exception/libexception.la

SUBDIRS = \
    . \
    test
