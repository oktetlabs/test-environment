#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT(rcfapi, 0.02, Elena.Vengerova@oktetlabs.ru)

AC_CONFIG_SRCDIR([rcf_api.h])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)


CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)

AM_CONFIG_HEADER(config.h)
AC_CHECK_HEADERS([pthread.h fcntl.h])

AH_TEMPLATE(PTHREAD_SETSPECIFIC_BUG,
            [Workaround for pthread_setspecific() bug present in RedHat 9.0])

#Checking for the nessecerity of workaround for RH-specific pthread bug

if test x$ac_cv_header_pthread_h = x"yes" -a -e /etc/redhat-release ; then
  str=`cat /etc/redhat-release`
  if test "$str" = "Red Hat Linux release 9.0 (Shrike)" ; then
    AC_DEFINE(PTHREAD_SETSPECIFIC_BUG,1)
  fi
fi

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
