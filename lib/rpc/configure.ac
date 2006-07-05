#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([rpc], [1.0], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpc.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_PROG_CC
if test x$host != x${host%windows} ; then
    RANLIB=touch
else    
    AC_PROG_RANLIB
fi    

TE_LIB_SET

AC_CONFIG_FILES([Makefile])

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

TE_LIB_RESTORE

AC_OUTPUT
