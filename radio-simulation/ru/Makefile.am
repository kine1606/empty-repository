bin_PROGRAMS = ru_simulation

# Build ru as a library
lib_LTLIBRARIES = libru.la
libru_la_SOURCES = src/RadioUnit.cc

libru_la_CPPFLAGS = -I$(srcdir)/inc

ru_simulation_SOURCES = src/main.cc

# guide the compiler where to find libs header
ru_simulation_CPPFLAGS = \
    -I$(srcdir)/inc
    
# ru_simulation_CPPFLAGS = \
    # -I$(top_srcdir)/libraries/Database/inc \
    # -I$(top_srcdir)/libraries/Exception/inc \
    # -I$(top_srcdir)/libraries/Logger/inc \
    #   $(srcdir)/inc

# flag for all of the thing and all of error
ru_simulation_CXXFLAGS = -std=c++20 

# link the libs: here we need database, exception, logger
# ru_simulation_LDADD = \
#     $(top_builddir)/Libraries/Database/libdatabase.la \
#     $(top_builddir)/Libraries/Exception/libexception.la \
#     $(top_builddir)/Libraries/Logger/liblogger.la \
#     $(builddir)/libru.la

ru_simulation_LDADD = $(builddir)/libru.la

# Compiler looking for test
SUBDIRS = \
    . \
    test
