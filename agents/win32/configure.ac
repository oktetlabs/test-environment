#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-win32], [0.3], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([win32.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)

AC_CANONICAL_HOST

AC_SUBST([host])
AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CPPFLAGS])
AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}
LDFLAGS=${LDFLAGS}" "${TE_LDFLAGS}

AC_CONFIG_HEADERS([config.h])

AC_CHECK_HEADERS([\
assert.h \
sys/types.h \
string.h \
time.h \
sys/time.h \
sys/resource.h \
sys/socket.h \
unistd.h \
pthread.h \
semaphore.h \
])
AC_C_INLINE

AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")

if test -n "$with_rcf_rpc" ; then
    LIBS="${LDFLAGS} -lrpc ${LIBS}"
    AH_TEMPLATE([xdr_int8_t], [RPC XDR for signed 8bit width type])
    AC_CHECK_FUNC([xdr_int8_t], [],
        [AC_CHECK_FUNC([xdr_char],
            [AC_DEFINE([xdr_int8_t], [xdr_char])],
            [AC_ERROR([There is no xdr_uint8_t or xdr_char!])])])

    AH_TEMPLATE([xdr_uint8_t], [RPC XDR for unsigned 8bit width type])
    AC_CHECK_FUNC([xdr_uint8_t], [],
        [AC_CHECK_FUNC([xdr_u_char],
            [AC_DEFINE([xdr_uint8_t], [xdr_u_char])],
            [AC_ERROR([There is no xdr_uint8_t or xdr_u_char!])])])

    AH_TEMPLATE([xdr_int16_t], [RPC XDR for signed 16bit width type])
    AC_CHECK_FUNC([xdr_int16_t], [],
        [AC_CHECK_FUNC([xdr_short],
            [AC_DEFINE([xdr_int16_t], [xdr_short])],
            [AC_ERROR([There is no xdr_uint16_t or xdr_short!])])])

    AH_TEMPLATE([xdr_uint16_t], [RPC XDR for unsigned 16bit width type])
    AC_CHECK_FUNC([xdr_uint16_t], [],
        [AC_CHECK_FUNC([xdr_u_short],
            [AC_DEFINE([xdr_uint16_t], [xdr_u_short])],
            [AC_ERROR([There is no xdr_uint16_t or xdr_u_short!])])])

    AH_TEMPLATE([xdr_int32_t], [RPC XDR for signed 32bit width type])
    AC_CHECK_FUNC([xdr_int32_t], [],
        [AC_CHECK_FUNC([xdr_int],
            [AC_DEFINE([xdr_int32_t], [xdr_int])],
            [AC_ERROR([There is no xdr_uint32_t or xdr_int!])])])

    AH_TEMPLATE([xdr_uint32_t], [RPC XDR for unsigned 32bit width type])
    AC_CHECK_FUNC([xdr_uint32_t], [],
        [AC_CHECK_FUNC([xdr_u_int],
            [AC_DEFINE([xdr_uint32_t], [xdr_u_int])],
            [AC_ERROR([There is no xdr_uint32_t or xdr_u_int!])])])

    AH_TEMPLATE([xdr_uint64_t], [RPC XDR for unsigned 64bit width type])
    AC_CHECK_FUNC([xdr_uint64_t], [],
        [AC_CHECK_FUNC([xdr_u_int64_t],
            [AC_DEFINE([xdr_uint64_t], [xdr_u_int64_t])],
            [AC_ERROR([There is no xdr_uint64_t or xdr_u_int64_t!])])])
fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
