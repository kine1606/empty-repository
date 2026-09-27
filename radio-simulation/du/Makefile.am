bin_PROGRAMS = du_simulation

# Build du as a library
lib_LTLIBRARIES = libdu.la
libdu_la_SOURCES = src/DistributedUnit.cc
libdu_la_CPPFLAGS = \
    -I$(srcdir)/inc

du_simulation_SOURCES = src/main.cc

# guide the compiler where to find libs header
du_simulation_CPPFLAGS = \
    -I$(srcdir)/inc

# du_simulation_CPPFLAGS = \
#     -I$(top_srcdir)/libraries/Database/inc \
#     -I$(top_srcdir)/libraries/Exception/inc \
#     -I$(top_srcdir)/libraries/Logger/inc \
#       $(srcdir)/inc

# flag for all of the thing and all of error
# du_simulation_CXXFLAGS = -std=c++20 -Wall -Wextra
du_simulation_CXXFLAGS = -std=c++20 

# link the libs: here we need database, exception, logger
# du_simulation_LDADD = \
#     $(top_builddir)/libraries/Database/libdatabase.la \
#     $(top_builddir)/libraries/Exception/libexception.la \
#     $(top_builddir)/libraries/Logger/liblogger.la \
#     $(builddir)/libdu.la

du_simulation_LDADD = $(builddir)/libdu.la

# Compiler looking for test
SUBDIRS = \
    . \
    test
