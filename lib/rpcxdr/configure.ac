#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([rpcxdr],[0.02],[Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpc_xdr.h])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CPPFLAGS])
AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])

AC_CHECK_HEADERS([expat.h rpc/types.h])
AC_CHECK_HEADERS([rpc/xdr.h], [], [], [
#if HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif
])

LIBS=${TE_LDFLAGS}
AC_CHECK_LIB([rpc], [xdr_bytes], 
             [LIBS="${LIBS} -lrpc"],
             [LIBS="${LIBS} -lc"])

AH_TEMPLATE([x_putint32], [Put 32-bit integer to the stream])
AC_CHECK_MEMBER(struct XDR.x_ops->x_putint32, [],
    [AC_DEFINE([x_putint32],[x_putlong])], [
#if HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif
#if HAVE_RPC_XDR_H
#include <rpc/xdr.h>
#endif
])

AH_TEMPLATE([x_getint32], [Get 32-bit integer to the stream])
AC_CHECK_MEMBER(struct XDR.x_ops->x_getint32, [],
    [AC_DEFINE([x_getint32],[x_getlong])], [
#if HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif
#if HAVE_RPC_XDR_H
#include <rpc/xdr.h>
#endif
])

AM_CONDITIONAL(DIST_RPCDEFS, test -n "$with_rpcdefs")
RPCDEFS=
RPCDEFS_DIST_FILES=
for i in $with_rpcdefs ; do 
    TMP=`basename $i`
    if test ! -e $i -a -e ${srcdir}/$TMP ; then
        i=${srcdir}/$TMP ;
    fi
    RPCDEFS="$RPCDEFS $i"
    RPCDEFS_DIST_FILES="$RPCDEFS_DIST_FILES $TMP"
done
AC_SUBST(RPCDEFS)
AC_SUBST(RPCDEFS_DIST_FILES)
 
AH_TEMPLATE([xdr_int8_t], [RPC XDR for signed 8bit width type])
AC_CHECK_FUNC([xdr_int8_t], [],
    [AC_CHECK_FUNC([xdr_char],
        [AC_DEFINE([xdr_int8_t], [xdr_char])],
        [AC_MSG_ERROR([There is no xdr_uint8_t or xdr_char!])])])

AH_TEMPLATE([xdr_uint8_t], [RPC XDR for unsigned 8bit width type])
AC_CHECK_FUNC([xdr_uint8_t], [],
    [AC_CHECK_FUNC([xdr_u_char],
        [AC_DEFINE([xdr_uint8_t], [xdr_u_char])],
        [AC_MSG_ERROR([There is no xdr_uint8_t or xdr_u_char!])])])

AH_TEMPLATE([xdr_int16_t], [RPC XDR for signed 16bit width type])
AC_CHECK_FUNC([xdr_int16_t], [],
    [AC_CHECK_FUNC([xdr_short],
        [AC_DEFINE([xdr_int16_t], [xdr_short])],
        [AC_MSG_ERROR([There is no xdr_uint16_t or xdr_short!])])])

AH_TEMPLATE([xdr_uint16_t], [RPC XDR for unsigned 16bit width type])
AC_CHECK_FUNC([xdr_uint16_t], [],
    [AC_CHECK_FUNC([xdr_u_short],
        [AC_DEFINE([xdr_uint16_t], [xdr_u_short])],
        [AC_MSG_ERROR([There is no xdr_uint16_t or xdr_u_short!])])])

AH_TEMPLATE([xdr_int32_t], [RPC XDR for signed 32bit width type])
AC_CHECK_FUNC([xdr_int32_t], [],
    [AC_CHECK_FUNC([xdr_int],
        [AC_DEFINE([xdr_int32_t], [xdr_int])],
        [AC_MSG_ERROR([There is no xdr_uint32_t or xdr_int!])])])

AH_TEMPLATE([xdr_uint32_t], [RPC XDR for unsigned 32bit width type])
AC_CHECK_FUNC([xdr_uint32_t], [],
    [AC_CHECK_FUNC([xdr_u_int],
        [AC_DEFINE([xdr_uint32_t], [xdr_u_int])],
        [AC_MSG_ERROR([There is no xdr_uint32_t or xdr_u_int!])])])

AH_TEMPLATE([xdr_uint64_t], [RPC XDR for unsigned 64bit width type])
AC_CHECK_FUNC([xdr_uint64_t], [],
    [AC_CHECK_FUNC([xdr_u_int64_t],
        [AC_DEFINE([xdr_uint64_t], [xdr_u_int64_t])],
        [AC_MSG_ERROR([There is no xdr_uint64_t or xdr_u_int64_t!])])])

AC_CHECK_LIB([expat], [XML_ParserCreate], [],
             AC_MSG_WARN([Cannot find expat]))

AC_CONFIG_HEADER(config.h)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
