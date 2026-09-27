bin_PROGRAMS = terminal_simulation

# Build du as a library
lib_LTLIBRARIES = libterminal.la
libterminal_la_SOURCES = \
    src/Terminal.cc

libterminal_la_CPPFLAGS = \
    -I$(srcdir)/inc

terminal_simulation_SOURCES = \
    src/main.cc

# guide the compiler where to find libs header
terminal_simulation_CPPFLAGS = \
    -I$(srcdir)/inc

# terminal_simulation_CPPFLAGS = \
#     -I$(top_srcdir)/libraries/Database/inc \
#     -I$(top_srcdir)/libraries/Exception/inc \
#     -I$(top_srcdir)/libraries/Logger/inc \
#       $(srcdir)/inc

# flag for all of the thing and all of error
# du_simulation_CXXFLAGS = -std=c++20 -Wall -Wextra
terminal_simulation_CXXFLAGS = -std=c++20 

# link the libs: here we need database, exception, logger
# du_simulation_LDADD = \
#     $(top_builddir)/libraries/Database/libdatabase.la \
#     $(top_builddir)/libraries/Exception/libexception.la \
#     $(top_builddir)/libraries/Logger/liblogger.la \
#     $(builddir)/libdu.la

terminal_simulation_LDADD = $(builddir)/libterminal.la

# Compiler looking for test
SUBDIRS = \
    . \
    test
