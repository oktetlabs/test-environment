#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(ta-win32, 0.3, Elena.Vengerova@oktetlabs.ru)
AC_CONFIG_SRCDIR([win32.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)

AC_CANONICAL_HOST

AC_SUBST(host)
AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS(time.h sys/time.h sys/resource.h semaphore.h)
AC_C_INLINE

if test -n "$with_cfg_win32_daemons" ; then
    DAEMONS_CFLAGS=-DCFG_WIN32_DAEMONS
    for i in $with_cfg_win32_daemons ; do
        case $i in 
            tftp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TFTP_SERVER" ;;
            ftp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_FTP_SERVER" ;;
            dns)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DNS_SERVER" ;;
            echo)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_ECHO_SERVER" ;;
            todudp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TODUDP_SERVER" ;;
            dhcp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DHCP_SERVER" ;
                   DAEMONS_LDADD="-ldhcpctl -lomapi -ldst" ;;
            *)  ;; 
        esac
    done
fi
AC_SUBST(DAEMONS_CFLAGS)
AC_SUBST(DAEMONS_LDADD)

AM_CONDITIONAL(CFG_WIN32_DAEMONS, test -n "$with_cfg_win32_daemons")
AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")

AC_LIBLTDL_INSTALLABLE
# Substitute INCLTDL and LIBLTDL in the Makefiles
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)
# Check for dlopen support
AC_LIBTOOL_DLOPEN
# Configure libtool
AC_PROG_LIBTOOL
# Configure libltdl
enable_ltdl_install=yes
AC_CONFIG_SUBDIRS(libltdl)
                            
AC_CONFIG_FILES(Makefile)

AC_OUTPUT
