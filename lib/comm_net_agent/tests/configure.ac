
# Process this file with autoconf to produce a configure script.
AC_INIT([comm_net_agent_tests],[1.0],
  [Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>])
AC_REVISION([$Id: configure.ac,v 1.1 2003/10/28 09:35:42 arybchik Exp $])
AC_CONFIG_SRCDIR([init01.c])

AC_CONFIG_AUX_DIR([../../../auxdir])
AC_CONFIG_MACRO_DIR([../../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CONFIG_HEADERS([include/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([pthread],[main],[],[],[])
ac_cv_lib_pthread=ac_cv_lib_pthread_main


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_CHECK_TYPES([pthread_barrier_t])

# Checks for library functions.
AC_CHECK_FUNCS([strerror nanosleep])
dnl AC_CHECK_LIB(pthread, pthread_barrier_init)
dnl AC_CHECK_FUNCS([pthread_barrier_init])

AC_CONFIG_FILES([Makefile lib/Makefile])
AC_OUTPUT
