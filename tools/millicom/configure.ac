#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])

AC_INIT([millicom], [0.02], [Konstantin.Abramenko@oktetlabs.ru])

AC_CONFIG_AUX_DIR([../../auxdir])
AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CONFIG_SRCDIR([millicom.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
#AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lm':
#AC_CHECK_LIB([m], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h termio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL

AC_CONFIG_FILES([Makefile])

AC_OUTPUT


CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}

if test "x$TE_BASE" != "x"
then
    CPPFLAGS=${CPPFLAGS}" "-I${TE_BASE}/include
fi

AC_HEADER_STDC

AC_OUTPUT
