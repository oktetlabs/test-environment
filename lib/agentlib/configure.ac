#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([agentlib],[0.01],[Artem.Andreev@oktetlabs.ru])

AC_CONFIG_SRCDIR([agentlib.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

AC_ARG_ENABLE([ftp],
              AS_HELP_STRING([--disable-ftp],
                             [disable FTP support (enabled by default)]),
              [ac_cv_enable_ftp=$enableval],
              [ac_cv_enable_ftp=yes])
AC_CACHE_CHECK([whether to enable FTP support],
               [ac_cv_enable_ftp], [ac_cv_enable_ftp=yes])
if test x"$ac_cv_enable_ftp" != xno
then
    AC_DEFINE([ENABLE_FTP], [], [Whether to enable FTP support])
fi

AC_ARG_ENABLE([telephony],
              AS_HELP_STRING([--enable-telephony],
                             [enable telephony support (disabled by default)]),
              [ac_cv_enable_telephony=$enableval],
              [ac_cv_enable_telephony=no])
AC_CACHE_CHECK([whether to enable telephony support],
               [ac_cv_enable_telephony], [ac_cv_enable_telephony=no])
if test x"$ac_cv_enable_telephony" != xno
then
    AC_DEFINE([ENABLE_TELEPHONY], [], [Whether to enable telephony support])
fi

AC_ARG_ENABLE([power-sw],
              AS_HELP_STRING([--disable-power-sw],
                             [disable power switch management support (enabled by default)]),
              [ac_cv_enable_power_sw=$enableval],
              [ac_cv_enable_power_sw=yes])
AC_CACHE_CHECK([whether to enable power switch management support],
               [ac_cv_enable_power_sw], [ac_cv_enable_power_sw=yes])
if test x"$ac_cv_enable_power_sw" != xno
then
    AC_DEFINE([ENABLE_POWER_SW], [], [Whether to enable power switch management support])

dnl
dnl Terminal devices
dnl
    AC_CHECK_HEADERS([termios.h])

dnl
dnl Parport devices
dnl
   AC_CHECK_HEADERS([linux/ppdev.h])
fi

AC_ARG_ENABLE([dlfcn-lookup],
              AS_HELP_STRING([--enable-dlfcn-lookup],
                             [enable symbol lookup via dlsym() in addition to explicit symbol tables (disabled by default)]),
              [ac_cv_enable_dlfcn_lookup=$enableval],
              [ac_cv_enable_dlfcn_lookup=no])
AC_CACHE_CHECK([whether to enable dlsym() symbol lookup],
               [ac_cv_enable_dlfcn_lookup], [ac_cv_enable_dlfcn_lookup=no])
if test x"$ac_cv_enable_dlfcn_lookup" != xno; then
     AC_CHECK_FUNC([dladdr])
     AC_DEFINE([ENABLE_DLFCN_LOOKUP], [], [Whether to enable dlsym()-based lookup])
fi

AC_ARG_ENABLE([upnp],
              AS_HELP_STRING([--enable-upnp],
                             [enable upnp support (disabled by default)]),
              [ac_cv_enable_upnp=$enableval],
              [ac_cv_enable_upnp=no])
AC_CACHE_CHECK([whether to enable upnp support],
               [ac_cv_enable_upnp], [ac_cv_enable_upnp=no])
if test x"$ac_cv_enable_upnp" != xno
then
    AC_DEFINE([ENABLE_UPNP], [], [Whether to enable upnp support])
fi

AC_ARG_WITH([name],,,[with_name=$PACKAGE_NAME])
AC_SUBST([with_name])

if test "x$with_name" != "x$PACKAGE_NAME"
then
   AC_SUBST([include_subdir], [$with_name])
else
   AC_SUBST([include_subdir], [.])
fi

AC_CONFIG_HEADERS([agentlib_config.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
