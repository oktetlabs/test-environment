#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([rcfpch], [0.02], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([rcf_pch.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
if test x$host != x${host%windows} ; then
    RANLIB=touch
else    
    AC_PROG_RANLIB
fi    

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

AC_SUBST(host)

TE_LIB_SET

AC_CONFIG_HEADER(package.h)

AC_ARG_WITH([sniffers],
            AS_HELP_STRING([--with-sniffers],
                           [enable sniffers (default is NO)]),
            [with_sniffers=yes],
            [with_sniffers=""])

AM_CONDITIONAL(CFG_SNIFFERS_SUPPORT, test -n "$with_sniffers")
if test -n "$with_sniffers"; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_SNIFFERS"
fi

AC_SUBST([EXTRA_CPPFLAGS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
