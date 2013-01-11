#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([rgt],[1.00],[Oleg.Kravtsov@oktetlabs.ru])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_SRCDIR([rgt-core/rgt_core.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_APP_SET

# Checks for libraries.

# By default we include log filtering stuff
AC_ARG_WITH([log_filter],
            AS_HELP_STRING([--with-log-filter],
                           [enable filtering support in RGT core]),
            [ac_cv_with_log_filter=$withval], [ac_cv_with_log_filter=yes])

if test "x$ac_cv_with_log_filter" = "xyes"
then
    AC_CHECK_LIB([tcl], [Tcl_CreateInterp],
                 ac_cv_lib_tcl=yes, ac_cv_lib_tcl=no, -lm -ldl)
    AC_CHECK_HEADER([tcl.h], ac_cv_header_tcl_h=yes, ac_cv_header_tcl_h=no)

    if test x$ac_cv_lib_tcl = xno || test x$ac_cv_header_tcl_h = xno
    then
        # In Debian distributive TCL is placed under some untrivial 
        # directories
        # Sets TCL_BIN_DIR
        SC2_PATH_TCLCONFIG
        # Sources tclConfig.sh, substs TCL_{BIN,SRC,LIB}_DIR
        SC2_LOAD_TCLCONFIG
        if test -n "$TCL_INCLUDE_SPEC"; then 
            TCL_CFLAGS=$TCL_INCLUDE_SPEC
        elif test -n "$TCL_INC_DIR"; then 
            TCL_CFLAGS="-I$TCL_INC_DIR"
        fi    
    else
        TCL_LIB_SPEC=-ltcl
        TCL_CFLAGS=
    fi
    AC_DEFINE([WITH_LOG_FILTER], 1, [Embed log fitering stuff in RGT core])
else
    TCL_LIB_SPEC=
    TCL_CFLAGS=
fi

WITH_LOG_FILTER=$ac_cv_with_log_filter
AC_SUBST([WITH_LOG_FILTER])

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
                           [specify location for glib-2.0 installation]),
            [ac_cv_with_glib=$withval], [ac_cv_with_glib=])

if test "x$ac_cv_with_glib" != "x"
then
    OLD_PKG_CONFIG_PATH=$PKG_CONFIG_PATH
    # Find glib library at this path first
    PKG_CONFIG_PATH="$ac_cv_with_glib/lib/pkgconfig:$PKG_CONFIG_PATH"
    export PKG_CONFIG_PATH
fi

GLIB_REQUIRED_VERSION=2.4.8
AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION, [],
  AC_MSG_WARN("GLIB $GLIB_REQUIRED_VERSION or better is required.
 The latest version of GLIB is always available from ftp://ftp.gtk.org/
 The package can't be built without this library, so you are allowed to
 make only 'dist' target. You can specify a path to your local version 
 of glib by using --with-glib configure option."), )


if test "x$ac_cv_with_glib" != "x"
then
    # Reset pkg-config PATH to its original value
    PKG_CONFIG_PATH=$OLD_PKG_CONFIG_PATH
    export PKG_CONFIG_PATH
fi

xml_lib=
xml_lib_err="You should select only one XML library. "\
"Whether Libxml, or Expat"
AC_ARG_WITH([libxml],
            AS_HELP_STRING([--with-libxml],
                           [use XML parser from Libxml library]),
            [
                if test "x$xml_lib" != "x"
                then
                    AC_MSG_ERROR("[$xml_lib_err]")
                fi
                xml_lib=libxml
            ])
            
AC_ARG_WITH([expat],
            AS_HELP_STRING([--with-expat],
                           [use XML parser from Expat library]),
            [
                if test "x$xml_lib" != "x"
                then
                    AC_MSG_ERROR("[$xml_lib_err]")
                fi
                xml_lib=expat
            ])

if test "x$xml_lib" = "x"; then
    AC_MSG_WARN("[no XML parser library specified, libxml will be used]")
    xml_lib=libxml
fi

AS_MESSAGE([checking XML Parser library...])

if test "x$xml_lib" = "xlibxml" || test "x$xml_lib" = "x"
then
    # Use Libxml
    if test "x$xml_lib" = "x" ; then
        TE_PATH_LIBXML2([NOTE])
    else
        TE_PATH_LIBXML2
    fi

    XML_PARSER_CFLAGS=${XML2_CFLAGS}
    XML_PARSER_LIBS=${XML2_LIBS}
fi

if test "x$xml_lib" = "xexpat" || 
   (test "x$xml_lib" = "x" && test "x$XML_PARSER_LIBS" = "x")
then
    # Use Expat
    TE_PATH_EXPAT

    XML_PARSER_CFLAGS=${EXPAT_CFLAGS}
    XML_PARSER_LIBS=${EXPAT_LIBS}
fi

AC_SUBST(XML_PARSER_CFLAGS)
AC_SUBST(XML_PARSER_LIBS)

TE_HEADERS_PATH=""
AC_ARG_WITH([te-headers-path],
            AS_HELP_STRING([--with-te-headers-path=[PATH]],
                           [specify location of TE headers]),
            CFLAGS="$CFLAGS -I$withval")


CFLAGS=${CFLAGS}" "${XML2_CFLAGS}
AC_CHECK_MEMBER([struct _xmlSAXHandler._private], [], [], 
                [#include <libxml/parser.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler.startElementNs], [], [], 
                [#include <libxml/parser.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler.endElementNs], [], [], 
                [#include <libxml/parser.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler.serror], [], [], 
                [#include <libxml/parser.h>])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/tmpls/Makefile
                 rgt-cfg/Makefile
                 rgt-cfg/tmpls/Makefile
                 rgt-core/Makefile
                 rgt-core/rgt-conv
                 rgt-format/Makefile
                 rgt-format/xml2html-multi/Makefile
                 rgt-format/xml2html-multi/tmpls/Makefile
                 rgt-format/xml2html-multi/tmpls-simple/Makefile
                 rgt-format/xml2html-multi/images/Makefile
                 rgt-format/xml2html-multi/misc/Makefile
                 rgt-format/xml2html/Makefile
                 rgt-format/xml2html/tmpls/Makefile
                 rgt-format/xml2html/images/Makefile
                 rgt-format/xml2html/misc/Makefile
                 rgt-format/xml2text/Makefile
                 rgt-format/xml2text/tmpls/Makefile
                 rgt-idx/Makefile
                 rgt-dump/Makefile
                 rgt-xml-merge/Makefile
                 rgt-pdml2xml/Makefile
])
#                 rgt-raw2xml/Makefile
#                 rgt-raw2xml/include/Makefile
#                 rgt-raw2xml/lib/Makefile
#                 rgt-raw2xml/lua/Makefile
#                 rgt-raw2xml/lua/loop/Makefile
#                 rgt-raw2xml/lua/loop/collection/Makefile
#                 rgt-raw2xml/lua/rgt/Makefile
#                 rgt-raw2xml/lua/rgt/node/Makefile

if test "x$TE_INSTALL" != "x"
then
    EXTRA_CPPFLAGS=${EXTRA_CPPFLAGS}" "-I${TE_INSTALL}/include
fi

## Actually, when rgt is build as a tool in common build tree
## configure is called before all TE headers are installed, so that 
## we can do nothing else than get them from source directory
## (it's ugly but...)
if test "x$TE_BASE" != "x"
then
    EXTRA_CPPFLAGS=${EXTRA_CPPFLAGS}" "-I${TE_BASE}/include
fi

# Large (>2Gb) log files support
AC_ARG_WITH([large_logs],
            [AS_HELP_STRING([--with-large-logs],
              [Enable support for large logs in RGT. \
Default - disabled.])],
            [if test "x${withval}" = "xyes"; then
                 with_large_logs=yes;
             fi], [])

if test "x$with_large_logs" = "xyes"; then
    EXTRA_CPPFLAGS="${EXTRA_CPPFLAGS} -D_LARGEFILE64_SOURCE \
                    -D_FILE_OFFSET_BITS=64"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

dnl Set some constants for the package
AC_SUBST([EXTRA_CPPFLAGS])

TE_APP_RESTORE

AC_OUTPUT
