#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(tester, 0.4, Andrew.Rybchenko@oktetlabs.ru)
AC_CONFIG_SRCDIR([tester.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL


# Checks for libraries
LIBXML_CONFIG_PREFIX=""
AC_ARG_WITH(libxml-prefix,
            [  --with-libxml-prefix=[PFX]	Specify location of libxml config (not including bin directory)],
            LIBXML_CONFIG_PREFIX=$withval
)
if test "x$LIBXML_CONFIG_PREFIX" != "x" ; then
    XML_CONFIG=${LIBXML_CONFIG_PREFIX}/bin/xml2-config
else
    XML_CONFIG=xml2-config
fi

AC_MSG_CHECKING(for libxml2 library)
if ${XML_CONFIG} --libs print >/dev/null 2>&1
then
    XML2_LIBS="$XML2_LIBS `$XML_CONFIG --libs`"
    XML2_CFLAGS="$XML2_CFLAGS `$XML_CONFIG --cflags`"
    AC_MSG_RESULT(found)
else
    AC_MSG_ERROR([Could not find libxml2 anywhere, check ftp://xmlsoft.org/.])
fi

AC_SUBST(XML2_CFLAGS)
AC_SUBST(XML2_LIBS)


AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}


AM_CONFIG_HEADER(config.h)

AC_HEADER_STDC
AC_CHECK_HEADERS([\
unistd.h \
strings.h \
pthread.h \
sys/types.h \
sys/wait.h \
sys/queue.h \
])


AC_CONFIG_FILES(Makefile)

AC_OUTPUT
