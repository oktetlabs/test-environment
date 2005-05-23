#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])

AC_INIT(rgt, 1.00, Oleg.Kravtsov@oktetlabs.ru)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([rgt-core/rgt_core.c])
AC_CONFIG_AUX_DIR([auxdir])
AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_PROGS(XSLT_PROC, [xsltproc sabcmd])

if test -z "$XSLT_PROC" ; then 
  AC_MSG_WARN([No one XSLT processor was found.
  You can download one of xslt processors from http://xmlsoft.org/
  Some utilities don't work without XSLT processor])
fi
AC_SUBST([XSLT_PROC])

# Checks for libraries.
AC_CHECK_LIB([tcl], [Tcl_CreateInterp],,,-lm -ldl)
AC_CHECK_HEADER([tcl.h], ac_cv_header_tcl_h=yes, ac_cv_header_tcl_h=no)

if test x$ac_cv_lib_tcl = xno || test x$ac_cv_header_tcl_h = xno
then
    # In Debian distributive TCL is placed under some untrivial directories
    # Sets TCL_BIN_DIR
    SC2_PATH_TCLCONFIG
    # Sources tclConfig.sh, substs TCL_{BIN,SRC,LIB}_DIR
    SC2_LOAD_TCLCONFIG
    TCL_CFLAGS=-I$TCL_INC_DIR
else
    TCL_LIB_SPEC=-ltcl
    TCL_CFLAGS=
fi
AC_SUBST([TCL_LIB_SPEC])
AC_SUBST([TCL_CFLAGS])

AC_CHECK_LIB([popt], [poptGetContext], 
             ac_cv_lib_popt=yes, ac_cv_lib_popt=no)
if test "x$ac_cv_lib_popt" = "xno"
then
    AC_MSG_ERROR("[RGT utility requires popt library to be installed.]")
fi

AC_FUNC_OBSTACK
if test "x$ac_cv_func_obstack" = "xno"
then
    AC_MSG_ERROR("[RGT utility requires obstack feature to be installed.]")
fi

AC_C_BIGENDIAN

AC_ARG_WITH([glib],
            AS_HELP_STRING([--with-glib=[PFX]],
                           [Specify location for glib-2.0 installation]),
              [ac_cv_with_glib=$withval], [ac_cv_with_glib=])

if test "x$ac_cv_with_glib" != "x"
then
    OLD_PKG_CONFIG_PATH=$PKG_CONFIG_PATH
    # Find glib library at this path first
    PKG_CONFIG_PATH="$ac_cv_with_glib/lib/pkgconfig:$PKG_CONFIG_PATH"
    export PKG_CONFIG_PATH
fi

GLIB_REQUIRED_VERSION=2.0.0
AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION, [],
  AC_MSG_WARN("GLIB $GLIB_REQUIRED_VERSION or better is required. 
 The latest version of GLIB is always available from ftp://ftp.gtk.org/
 The package can't be built without this library, so you are allowed to 
 make only 'dist' target."), )


if test "x$ac_cv_with_glib" != "x"
then
    # Reset pkg-config APTH to its original value
    PKG_CONFIG_PATH=$OLD_PKG_CONFIG_PATH
    export PKG_CONFIG_PATH
fi

LIBXML_CONFIG_PREFIX=""
AC_ARG_WITH([libxml-prefix],
    [--with-libxml-prefix=[PFX] \
     Specify location of libxml config (not including bin directory)],
    LIBXML_CONFIG_PREFIX=$withval)
if test "x$LIBXML_CONFIG_PREFIX" != "x"
then
    XML_CONFIG=${LIBXML_CONFIG_PREFIX}/bin/xml2-config
else
    XML_CONFIG=xml2-config
fi

TE_HEADERS_PATH=""
AC_ARG_WITH([te-headers-path],
   [  --with-te-headers-path=[PATH]        Specify location of TE headers],
   CFLAGS="$CFLAGS -I$withval")


AC_MSG_CHECKING(for libxml2 library)
if ${XML_CONFIG} --libs print > /dev/null 2>&1
then
    XML2_LIBS="$XML2_LIBS `$XML_CONFIG --libs`"
    XML2_CFLAGS="$XML2_CFLAGS `$XML_CONFIG --cflags`"
    AC_MSG_RESULT(found)
else
    AC_MSG_ERROR([Could not find libxml2, check ftp://xmlsoft.org/.])
fi

AC_SUBST([XML2_CFLAGS])
AC_SUBST([XML2_LIBS])

CFLAGS=${CFLAGS}" "${XML2_CFLAGS}
AC_CHECK_MEMBERS([\
struct _xmlSAXHandler._private, \
struct _xmlSAXHandler.startElementNs, \
struct _xmlSAXHandler.endElementNs, \
struct _xmlSAXHandler.serror \
], [], [], [#include <libxml/parser.h>])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/tmpls/Makefile
                 rgt-core/Makefile 
                 rgt-core/rgt-filter-prepare
                 rgt-core/rgt-conv
                 rgt-core/tests/Makefile
                 rgt-core/tests/filter-prepare/Makefile
                 rgt-format/Makefile
                 rgt-format/xml2html-multi/Makefile
                 rgt-format/xml2html-multi/tmpls/Makefile
                 rgt-format/xml2html-multi/images/Makefile
                 rgt-format/xml2html-multi/misc/Makefile
                 rgt-format/xml2html/Makefile
                 rgt-format/xml2html/tmpls/Makefile
                 rgt-format/xml2html/images/Makefile
                 rgt-format/xml2html/misc/Makefile
                 rgt-format/xml2text/Makefile
                 rgt-format/xml2text/tmpls/Makefile])

CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}

if test "x$TE_INSTALL" != "x"
then
    CPPFLAGS=${CPPFLAGS}" "-I${TE_INSTALL}/include
fi

## Actually, when rgt is build as a tool in common build tree
## configure is called before all TE headers are installed, so that 
## we can do nothing else than get them from source directory
## (it's ugly but...)
if test "x$TE_BASE" != "x"
then
    CPPFLAGS=${CPPFLAGS}" "-I${TE_BASE}/include
fi


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([signal.h            \
                  sys/types.h         \
                  netinet/in.h        \
                  inttypes.h stdint.h \
                  stdlib.h            \
                  string.h strings.h  \
                  unistd.h            \
                  time.h              \
                  tcl.h               \
                  obstack.h           \
                  assert.h            \
                  ctype.h             \
                  te_raw_log.h        \
                  logger_defs.h       \
                  ])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

dnl Set some constants for the package
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])


AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

AC_OUTPUT
