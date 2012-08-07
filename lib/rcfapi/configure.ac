#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([rcfapi], [0.02], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([rcf_api.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

TE_LIB_SET

AC_CHECK_TOOL(AR, ar)

AC_CONFIG_HEADERS([config.h])

AH_TEMPLATE(PTHREAD_SETSPECIFIC_BUG,
    [Workaround for pthread_setspecific() bug present in RedHat 9.0])

#Checking for the nessecerity of workaround for RH-specific pthread bug

if test x$ac_cv_header_pthread_h = x"yes" -a -e /etc/redhat-release ; then
  str=`cat /etc/redhat-release`
  if test "$str" = "Red Hat Linux release 9.0 (Shrike)" ; then
    AC_DEFINE(PTHREAD_SETSPECIFIC_BUG,1)
  fi
fi

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
