#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([configurator],[0.02],[Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([conf_db.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_FIXED_LIBTOOL_RANLIB

TE_APP_SET

# Checks for libraries
TE_PATH_LIBXML2

CPPFLAGS=${XML2_CFLAGS}
AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])

#AM_CONDITIONAL(CONFIGURATOR_DEBUG, test -n "$with_tests")

AC_CONFIG_FILES([Makefile])

TE_CS_CPPFLAGS=

AC_ARG_WITH([conf_yaml],,
[if test "${withval}" = yes; then
   AC_SEARCH_LIBS([yaml_parser_load], [yaml])
   TE_CS_CPPFLAGS="-DWITH_CONF_YAML"
   with_conf_yaml=yes
 fi],
[with_conf_yaml=no])

AC_SUBST([TE_CS_CPPFLAGS])
AM_CONDITIONAL(WITH_CONF_YAML, test "$with_conf_yaml" = "yes")

#if test -n "$with_tests"; then
#    AC_SUBST([prefix])
#    AC_SUBST([srcdir])
#    AC_CONFIG_FILES([tests/Makefile tests/db/Makefile tests/paths.c \
#                    tests/rcf_emul/Makefile tests/suites/Makefile])
#fi

TE_APP_RESTORE

AC_OUTPUT
