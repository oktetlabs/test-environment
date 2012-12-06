#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([te-tools], [0.1], [Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_tools.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL
if test x$host != x${host%windows} ; then
    RANLIB=touch
fi    

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

AC_SUBST(host)

TE_LIB_SET

if test x$host = x${host%windows} ; then
    dnl
    dnl POSIX threads are provided by:
    dnl - -lpthread in Linux;
    dnl - -lc_r (-lkse) in FreeBSD;
    dnl - -lpthread in NetBSD ('pth' package).
    dnl
    AC_SEARCH_LIBS([pthread_create], [pthread kse c_r], [],
                   AC_MSG_ERROR([No POSIX threads]))
    AC_SEARCH_LIBS([pthread_atfork], [pthread kse c_r], [],
                   AC_MSG_ERROR([pthread_atfork is not supported]))

    dnl
    dnl POSIX semaphores are provided by:
    dnl - -lpthread on Linux and NetBSD 3.1;
    dnl - -lc_r on FreeBSD (5, 6);
    dnl - -lsemaphore on NetBSD 2.0 ('pthread-sem' package);
    dnl - -lrt on Solaris2 (SunOS 5.11).
    dnl
    AC_SEARCH_LIBS([sem_init], [pthread c_r semaphore rt], [],
                   AC_MSG_ERROR([No POSIX semaphores]))
fi

dnl
dnl Obtaining network info about host name (and/or service)
dnl
AC_CHECK_FUNCS([getaddrinfo])

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
