AC_PREREQ([2.69])
AC_INIT([radio-simulation], [1.0])

# AC_CONFIG_SRCDIR([])
# AC_CONFIG_HEADERS([])

AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CC
AC_PROG_CXX

# Check for archiver
AM_PROG_AR

# init Librariestools for linking libraries
LT_PREREQ([2.4])
LT_INIT



# =========================================================================
# Threading Support (Check this before gRPC modules)
# =========================================================================
AX_PTHREAD([], [
    AC_MSG_ERROR([POSIX threads support is required to build this project.])
])

# Wire up the compiler and linkegitr flags discovered by AX_PTHREAD
CC="$PTHREAD_CC"
CXX="$PTHREAD_CXX"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CXXFLAGS"
LIBS="$PTHREAD_LIBS $LIBS"

# =========================================================================
# Dependency Checks via pkg-config
# =========================================================================

# 1. Check for Protobuf
PKG_CHECK_MODULES([PROTOBUF], [protobuf >= 3.0.0], [], [
    AC_MSG_ERROR([protobuf missing or version too old. Please install libprotobuf-dev.])
])

# 2. Check for gRPC (C++)
PKG_CHECK_MODULES([GRPC], [grpc++ >= 1.0.0], [], [
    AC_MSG_ERROR([grpc++ missing. Please install libgrpc++-dev.])
])

# 3. Check for Google 
PKG_CHECK_MODULES([GTEST], [gtest >= 1.8.0], [], [
    AC_MSG_ERROR([gtest missing. Please install libgtest-dev.])
])

AM_CONDITIONAL([HAVE_GTEST], [true])

# =========================================================================
# Check for Code Generator Binaries
# =========================================================================
AC_PATH_PROG([PROTOC], [protoc])
AS_IF([test -z "$PROTOC"], [AC_MSG_ERROR([protoc not found.])])

AC_PATH_PROG([GRPC_CPP_PLUGIN], [grpc_cpp_plugin])
AS_IF([test -z "$GRPC_CPP_PLUGIN"], [AC_MSG_ERROR([grpc_cpp_plugin not found.])])

# =========================================================================
# Update whenever adding something new here 
# =========================================================================
AC_CONFIG_FILES([
    Makefile

    du/Makefile
    du/test/Makefile

    ru/Makefile
    ru/test/Makefile

    terminal/Makefile
    terminal/test/Makefile
    libraries/Makefile

    libraries/database/Makefile
    libraries/database/test/Makefile

    libraries/grpc/Makefile
    libraries/proto/Makefile
    
    libraries/file-manager/Makefile
    libraries/file-manager/test/Makefile

    libraries/exception/Makefile
    libraries/exception/test/Makefile

    libraries/logger/Makefile
    libraries/logger/test/Makefile

    libraries/mailbox/Makefile
    libraries/mailbox/test/Makefile
    
    libraries/shared-function/Makefile
    libraries/shared-function/test/Makefile
])

AC_OUTPUT
