lib_LTLIBRARIES = libexception.la  

#Where source files are
libexception_la_SOURCES = \
    src/Exception.cc \
    src/FileManagerException.cc \
    src/DatabaseException.cc

#Where compiler can find the header
libexception_la_CPPFLAGS =   \
        -I$(srcdir)/inc      \
        -I$(srcdir)/../enums

SUBDIRS = \
    . \
    test
