#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([configurator], [0.02], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([conf_db.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

TE_APP_SET

# Checks for libraries
TE_PATH_LIBXML2

AC_CONFIG_HEADERS([config.h])

CPPFLAGS=${XML2_CFLAGS}
AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])

#AM_CONDITIONAL(CONFIGURATOR_DEBUG, test -n "$with_tests")

AC_CONFIG_FILES([Makefile])

#if test -n "$with_tests"; then
#    AC_SUBST([prefix])
#    AC_SUBST([srcdir])
#    AC_CONFIG_FILES([tests/Makefile tests/db/Makefile tests/paths.c \
#                    tests/rcf_emul/Makefile tests/suites/Makefile])
#fi

TE_APP_RESTORE

AC_OUTPUT
